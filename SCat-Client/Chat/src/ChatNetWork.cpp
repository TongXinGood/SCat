#include "../include/ChatNetWork.h"
#include "../../Other/include/UserSession.h"

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

void ChatNetWork::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    switch (type) {
    case MSG_CHAT_RESP:
        handleChatResp(obj);
        break;

    case MSG_CHAT_PUSH:
        handleChatPush(obj);
        break;

    default:
        break;      // 不是聊天模块的包，不管
    }
}

void ChatNetWork::handleChatResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code != ERR_OK) {
        qDebug() << "chat send failed, code:" << code;
        emit sendFailed(QString("发送失败 (code: %1)").arg(code));
        return;
    }

    ChatMessage msg;
    msg.from = UserSession::GetInstance().username();
    msg.to = obj["to"].toString();
    msg.content = obj["content"].toString();
    // 毫秒时间戳是 13 位数，超出了 int 范围。
    // 这里必须用 toVariant().toLongLong()，用 toInt() 会溢出变成 0
    msg.time = obj["time"].toVariant().toLongLong();
    msg.isSelf = true;

    if (!obj["delivered"].toBool())
        qDebug() << "peer offline, message not delivered:" << msg.to;

    emit messageSent(msg);
}

void ChatNetWork::handleChatPush(const QJsonObject& obj)
{
    ChatMessage msg;
    msg.from = obj["from"].toString();
    msg.to = obj["to"].toString();
    msg.content = obj["content"].toString();
    msg.time = obj["time"].toVariant().toLongLong();
    msg.isSelf = false;

    qDebug() << "chat received from" << msg.from;

    emit messageReceived(msg);
}