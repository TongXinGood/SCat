#include "../include/FriendManager.h"

FriendManager::FriendManager(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &FriendManager::onPacketReceived);
}

void FriendManager::requestFriendList()
{
    if (!net->isConnected()) {
        emit friendListFailed("未连接到服务器");
        return;
    }

    // 请求体是空的，服务端从 ClientSession 就知道是谁在问
    net->sendPacket(MSG_FRIEND_LIST_REQ, QJsonObject());
    qDebug() << "friend list request sent";
}

void FriendManager::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    switch (type) {
    case MSG_FRIEND_LIST_RESP:
        handleFriendListResp(obj);
        break;

    case MSG_FRIEND_STATUS_PUSH:
        handleFriendStatusPush(obj);
        break;

    default:
        break;      // 不是好友模块的包，不管
    }
}

void FriendManager::handleFriendListResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code != ERR_OK) {
        qDebug() << "friend list failed, code:" << code;
        emit friendListFailed(QString("拉取好友列表失败 (code: %1)").arg(code));
        return;
    }

    QJsonArray arr = obj["friends"].toArray();
    qDebug() << "friend list received:" << arr.size() << "friends";

    emit friendListReady(arr);
}

void FriendManager::handleFriendStatusPush(const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    bool online = obj["online"].toBool();

    qDebug() << "friend status changed:" << username << (online ? "online" : "offline");

    emit friendStatusChanged(username, online);
}