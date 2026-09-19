#include "../include/ChatNetWork.h"
#include "../../Other/include/UserSession.h"
#include "../../Other/include/ImageUtils.h"
#include <QJsonArray>
ChatNetWork::ChatNetWork(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &ChatNetWork::onPacketReceived);
}

void ChatNetWork::sendTextMessage(const QString& to, const QString& content)
{
    if (!net->isConnected()) {
        emit sendFailed("未连接到服务器");
        return;
    }

    if (to.isEmpty() || content.isEmpty())
        return;

    QJsonObject obj;
    obj["to"] = to;
    obj["content"] = content;

    net->sendPacket(MSG_CHAT_REQ, obj);
    qDebug() << "chat sent to" << to;
}

void ChatNetWork::sendImageMessage(const QString& to, const QString& imgName,
    const QByteArray& data, const QSize& size)
{
    if (!net->isConnected()) {
        emit sendFailed("未连接到服务器");
        return;
    }

    if (to.isEmpty() || imgName.isEmpty() || data.isEmpty())
        return;

    PendingImage pending;
    pending.imgName = imgName;
    pending.w = size.width();
    pending.h = size.height();
    pendingImages.enqueue(pending);

    // 包体本来就是 JSON，图片转成 base64 塞进字符串就行，包格式一个字节都不用改。
    // 压缩后 300KB 以内的图，base64 之后 400KB 左右，离 1MB 上限还有一倍余量，
    // 一个包发完，完全不用搞分块传输 —— 跟头像上传是同一个套路
    QJsonObject obj;
    obj["to"] = to;
    obj["kind"] = KIND_IMAGE;
    obj["content"] = QStringLiteral("[图片]");
    obj["image"] = QString::fromLatin1(data.toBase64());
    obj["w"] = size.width();
    obj["h"] = size.height();

    net->sendPacket(MSG_CHAT_REQ, obj);
    qDebug() << "image sent to" << to << data.size() << "bytes";
}

void ChatNetWork::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    switch (type) {
    case MSG_CHAT_RESP:
        handleChatResp(obj);
        break;

    case MSG_CHAT_PUSH:
        handleChatPush(obj);
        break;
    
    case MSG_OFFLINE_PUSH:
        handleOfflinePush(obj);
        break;
    
    default:
        break;      // 不是聊天模块的包，不管
    }
}

void ChatNetWork::handleChatResp(const QJsonObject& obj)
{
    int kind = obj["kind"].toInt();

    // 不管成功还是失败都得把队首取走，否则一次失败会让后面所有图片全部错位，
    // 张冠李戴比发不出去更难查
    PendingImage pending;
    if (kind == KIND_IMAGE && !pendingImages.isEmpty())
        pending = pendingImages.dequeue();

    int code = obj["code"].toInt();

    if (code != ERR_OK) {
        qDebug() << "chat send failed, code:" << code;
        emit sendFailed(QString("发送失败 (code: %1)").arg(code));
        return;
    }

    ChatMessage msg;
    msg.msgid = obj["msgid"].toString();
    msg.from = UserSession::GetInstance().username();
    msg.to = obj["to"].toString();
    msg.content = obj["content"].toString();
    msg.time = obj["time"].toVariant().toLongLong();
    msg.isSelf = true;
    msg.kind = kind;

    // 自己发的图早就在本地了，不用再从回执里解一遍
    msg.imgName = pending.imgName;
    msg.imgW = pending.w;
    msg.imgH = pending.h;

    if (!obj["delivered"].toBool())
        qDebug() << "peer offline, message not delivered:" << msg.to;

    emit messageSent(msg);
}

void ChatNetWork::fillImage(ChatMessage& msg, const QJsonObject& obj)
{
    if (msg.kind != KIND_IMAGE)
        return;

    QByteArray data = QByteArray::fromBase64(obj["image"].toString().toLatin1());

    msg.imgName = ImageUtils::saveToLocal(data);
    msg.imgW = obj["w"].toInt();
    msg.imgH = obj["h"].toInt();

    // 落盘失败（磁盘满、没权限）也不拦着消息上屏 ——
    // imgName 为空时气泡会显示"图片已失效"，起码知道对方发过东西
    if (msg.imgName.isEmpty())
        qDebug() << "save received image failed, msgid:" << msg.msgid;
}

void ChatNetWork::fillFile(ChatMessage& msg, const QJsonObject& obj)
{
    if (msg.kind != KIND_FILE)
        return;

    msg.fileId = obj["fileId"].toString();
    msg.fileName = obj["fileName"].toString();
    msg.fileSize = obj["fileSize"].toVariant().toLongLong();

    // 先摆在那儿等用户点下载。filePath 留空表示还没下到本地
    msg.fileState = FILE_STATE_READY;
}

void ChatNetWork::handleChatPush(const QJsonObject& obj)
{
    ChatMessage msg;
    msg.msgid = obj["msgid"].toString();
    msg.from = obj["from"].toString();
    msg.to = obj["to"].toString();
    msg.content = obj["content"].toString();
    msg.time = obj["time"].toVariant().toLongLong();
    msg.isSelf = false;
    msg.kind = obj["kind"].toInt();

    fillImage(msg, obj);
    fillFile(msg, obj);

    qDebug() << "chat received from" << msg.from;

    emit messageReceived(msg);
}

void ChatNetWork::handleOfflinePush(const QJsonObject& obj)
{
    QJsonArray arr = obj["msgs"].toArray();
    if (arr.isEmpty())
        return;

    QJsonArray acked;

    for (const QJsonValue& value : arr) {
        QJsonObject item = value.toObject();

        ChatMessage msg;
        msg.msgid = item["msgid"].toString();
        msg.from = item["from"].toString();
        msg.to = item["to"].toString();
        msg.content = item["content"].toVariant().toString();
        msg.time = item["time"].toVariant().toLongLong();
        msg.isSelf = false;
        msg.kind = item["kind"].toInt();

        fillImage(msg, item);
        fillFile(msg, item);

        // 走跟在线消息完全一样的路径：上层负责存本地 + 上屏。
        // 这里是同步调用，返回时消息已经进数据库了，所以下面确认是安全的
        emit messageReceived(msg);

        acked.append(msg.msgid);
    }

    // 确认收到，服务端才会把这批从 offline_msg 表里删掉
    QJsonObject ack;
    ack["msgids"] = acked;
    net->sendPacket(MSG_OFFLINE_ACK, ack);

    qDebug() << "offline messages received:" << acked.size();
}