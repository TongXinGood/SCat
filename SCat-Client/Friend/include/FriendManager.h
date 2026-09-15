#ifndef FRIENDMANAGER_H
#define FRIENDMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"

class NetWorkManager;

// 好友模块的逻辑层，只管收发包，不碰界面。
// 以后的加好友、删好友、搜好友也都收在这个类里。
class FriendManager : public QObject
{
    Q_OBJECT

public:
    explicit FriendManager(NetWorkManager* net, QObject* parent = nullptr);

public slots:
    // 向服务端要好友列表。不用传自己是谁，服务端从连接就知道
    void requestFriendList();
    void searchUser(const QString& username);
    void addFriend(const QString& username);
    void requestPendingList();
    void handleRequest(const QString& username, int action);

signals:
    void friendListReady(const QJsonArray& friends);
    void friendListFailed(const QString& reason);
    void friendStatusChanged(const QString& username, bool online);
    // 搜索结果
    void searchResult(const QString& username, const QString& nickname,const QString& avatar, int relation);
    void searchNotFound();

    // 申请发送的结果
    void addFriendResult(bool ok, const QString& reason);

    // 好友申请
    void pendingListReady(const QJsonArray& requests);
    void newRequestArrived(const QString& username, const QString& nickname,const QString& avatar);
    void requestHandled(bool ok, const QString& username, int action);
    void friendListChanged();
private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleFriendListResp(const QJsonObject& obj);
    void handleFriendStatusPush(const QJsonObject& obj);
    void handleSearchResp(const QJsonObject& obj);
    void handleAddResp(const QJsonObject& obj);
    void handlePendingListResp(const QJsonObject& obj);
    void handleReqPush(const QJsonObject& obj);
    void handleHandleResp(const QJsonObject& obj);

private:
    NetWorkManager* net;
};

#endif // FRIENDMANAGER_H