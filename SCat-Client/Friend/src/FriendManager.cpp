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
    
    case MSG_SEARCH_USER_RESP:
        handleSearchResp(obj);
        break;

    case MSG_FRIEND_ADD_RESP:
        handleAddResp(obj);
        break;
   
    case MSG_FRIEND_REQ_PUSH:
        handleReqPush(obj);
        break;

    case MSG_FRIEND_HANDLE_RESP:
        handleHandleResp(obj);
        break;

    case MSG_FRIEND_LIST_CHANGED:
        // 服务端说好友列表变了，重新拉一遍就完事，不用解析任何内容
        qDebug() << "friend list changed, reloading";
        emit friendListChanged();
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

void FriendManager::searchUser(const QString& username)
{
    if (!net->isConnected()) {
        emit searchNotFound();
        return;
    }

    QJsonObject obj;
    obj["username"] = username;

    net->sendPacket(MSG_SEARCH_USER_REQ, obj);
    qDebug() << "search user:" << username;
}

void FriendManager::addFriend(const QString& username)
{
    if (!net->isConnected()) {
        emit addFriendResult(false, "未连接到服务器");
        return;
    }

    QJsonObject obj;
    obj["username"] = username;

    net->sendPacket(MSG_FRIEND_ADD_REQ, obj);
    qDebug() << "add friend request:" << username;
}

void FriendManager::handleSearchResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code != ERR_OK) {
        qDebug() << "search failed, code:" << code;
        emit searchNotFound();      // 查无此人，或者别的错误，界面上都显示"没找到"
        return;
    }

    emit searchResult(obj["username"].toString(),
        obj["nickname"].toString(),
        obj["avatar"].toString(),
        obj["relation"].toInt());
}

void FriendManager::handleAddResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code == ERR_OK) {
        emit addFriendResult(true, QString());
        return;
    }

    QString reason;
    switch (code) {
    case ERR_ALREADY_FRIEND:  reason = "你们已经是好友了"; break;
    case ERR_CANNOT_ADD_SELF: reason = "不能添加自己"; break;
    case ERR_USER_NOT_FOUND:  reason = "该用户不存在"; break;
    default: reason = QString("申请失败 (code: %1)").arg(code); break;
    }

    emit addFriendResult(false, reason);
}

void FriendManager::requestPendingList()
{
    if (!net->isConnected())
        return;

    net->sendPacket(MSG_FRIEND_REQ_LIST_REQ, QJsonObject());
    qDebug() << "pending request list requested";
}

void FriendManager::handleRequest(const QString& username, int action)
{
    if (!net->isConnected()) {
        emit requestHandled(false, username, action);
        return;
    }

    QJsonObject obj;
    obj["username"] = username;
    obj["action"] = action;

    net->sendPacket(MSG_FRIEND_HANDLE_REQ, obj);
    qDebug() << "handle friend request:" << username << "action" << action;
}

void FriendManager::handlePendingListResp(const QJsonObject& obj)
{
    if (obj["code"].toInt() != ERR_OK) {
        qDebug() << "pending list failed, code:" << obj["code"].toInt();
        return;
    }

    QJsonArray arr = obj["requests"].toArray();
    qDebug() << "pending requests:" << arr.size();

    emit pendingListReady(arr);
}

void FriendManager::handleReqPush(const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    qDebug() << "new friend request from" << username;

    emit newRequestArrived(username,
        obj["nickname"].toString(),
        obj["avatar"].toString());
}

void FriendManager::handleHandleResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();
    QString username = obj["username"].toString();
    int action = obj["action"].toInt();

    if (code != ERR_OK)
        qDebug() << "handle request failed, code:" << code;

    // ERR_REQUEST_NOT_FOUND 也算"处理完了"—— 说明这条申请已经没了，
    // 界面上那一行本来就该消失
    bool ok = (code == ERR_OK || code == ERR_REQUEST_NOT_FOUND);

    emit requestHandled(ok, username, action);
}