#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QJsonObject>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"

class NetWorkManager;

// 设置模块的逻辑层，只管收发包，不碰界面。
// 以后的改头像也收在这个类里
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(NetWorkManager* net, QObject* parent = nullptr);

public slots:
    void saveNickname(const QString& nickname);
    void uploadAvatar(const QString& filePath);
    void downloadAvatar(const QString& avatar);


signals:
    void nicknameSaved(bool ok, const QString& nickname, const QString& reason);
    void avatarUploaded(bool ok, const QString& avatar, const QString& reason);
    void avatarDownloaded(const QString& avatar);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleNicknameResp(const QJsonObject& obj);
    void handleSetAvatarResp(const QJsonObject& obj);
    void handleGetAvatarResp(const QJsonObject& obj);

private:
    NetWorkManager* net;
};

#endif // SETTINGSMANAGER_H