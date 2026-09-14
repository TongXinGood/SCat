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

signals:
    void friendListReady(const QJsonArray& friends);
    void friendListFailed(const QString& reason);
    void friendStatusChanged(const QString& username, bool online);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleFriendListResp(const QJsonObject& obj);
    void handleFriendStatusPush(const QJsonObject& obj);

private:
    NetWorkManager* net;
};

#endif // FRIENDMANAGER_H