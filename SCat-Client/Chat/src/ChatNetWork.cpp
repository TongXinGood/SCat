#include "../include/ChatNetWork.h"
#include "../../Other/include/UserSession.h"
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

    if (!obj["delivered"].toBool())
        qDebug() << "peer offline, message not delivered:" << msg.to;

    emit messageSent(msg);
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