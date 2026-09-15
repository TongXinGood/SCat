#include "../include/SettingsManager.h"
#include "../../Other/include/AvatarUtils.h"
#include <QDebug>

SettingsManager::SettingsManager(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &SettingsManager::onPacketReceived);
}

void SettingsManager::saveNickname(const QString& nickname)
{
    if (!net->isConnected()) {
        emit nicknameSaved(false, nickname, "未连接到服务器");
        return;
    }

    QJsonObject obj;
    obj["nickname"] = nickname;

    net->sendPacket(MSG_SET_NICKNAME_REQ, obj);
    qDebug() << "save nickname:" << nickname;
}

void SettingsManager::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    switch (type) {
    case MSG_SET_NICKNAME_RESP:
        handleNicknameResp(obj);
        break;

    case MSG_SET_AVATAR_RESP:
        handleSetAvatarResp(obj);
        break;

    case MSG_GET_AVATAR_RESP:
        handleGetAvatarResp(obj);
        break;

    default:
        break;      // 不是设置模块的包，不管
    }
}

void SettingsManager::handleNicknameResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code == ERR_OK) {
        QString nickname = obj["nickname"].toString();
        qDebug() << "nickname saved:" << nickname;
        emit nicknameSaved(true, nickname, QString());
        return;
    }

    QString reason;
    switch (code) {
    case ERR_INVALID_PARAM: reason = "昵称不能为空，且不超过 16 个字"; break;
    case ERR_DB_ERROR:      reason = "服务器错误，请稍后再试"; break;
    default: reason = QString("保存失败 (code: %1)").arg(code); break;
    }

    emit nicknameSaved(false, QString(), reason);
}

void SettingsManager::uploadAvatar(const QString& filePath)
{
    if (!net->isConnected()) {
        emit avatarUploaded(false, QString(), "未连接到服务器");
        return;
    }

    QString error;
    QByteArray png = AvatarUtils::makeUploadData(filePath, error);

    if (png.isEmpty()) {
        emit avatarUploaded(false, QString(), error);
        return;
    }

    // 我们的包体本来就是 JSON，图片转成 base64 塞进字符串就行，
    // 包格式一个字节都不用改。256x256 的 PNG 通常几十 KB，
    // base64 之后也就一百来 KB，离 1MB 的上限还很远，一个包发完，
    // 完全不用搞分块传输
    QJsonObject obj;
    obj["data"] = QString::fromLatin1(png.toBase64());

    net->sendPacket(MSG_SET_AVATAR_REQ, obj);
    qDebug() << "upload avatar:" << png.size() << "bytes";
}

void SettingsManager::downloadAvatar(const QString& avatar)
{
    if (!net->isConnected() || avatar.isEmpty())
        return;

    QJsonObject obj;
    obj["avatar"] = avatar;

    net->sendPacket(MSG_GET_AVATAR_REQ, obj);
    qDebug() << "download avatar:" << avatar;
}

void SettingsManager::handleSetAvatarResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code != ERR_OK) {
        emit avatarUploaded(false, QString(),
            QString("上传失败 (code: %1)").arg(code));
        return;
    }

    emit avatarUploaded(true, obj["avatar"].toString(), QString());
}

void SettingsManager::handleGetAvatarResp(const QJsonObject& obj)
{
    QString avatar = obj["avatar"].toString();

    if (obj["code"].toInt() != ERR_OK) {
        qDebug() << "download avatar failed:" << avatar;
        return;      // 下不到就算了，界面会用默认头像兜底
    }

    QByteArray data = QByteArray::fromBase64(obj["data"].toString().toLatin1());

    if (AvatarUtils::saveToCache(avatar, data)) {
        qDebug() << "avatar cached:" << avatar << data.size() << "bytes";
        emit avatarDownloaded(avatar);
    }
}