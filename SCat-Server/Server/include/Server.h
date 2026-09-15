#pragma once

#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>
#include <QHash>
#include <QUuid>


class ClientSession;
class Database;
class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(Database* db,QObject* parent = nullptr);

    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onPacketReceived(ClientSession* from, quint16 type, const QJsonObject& obj);
    void onSessionClosed(ClientSession* session);


private:
    void handleLogin(ClientSession* from, const QJsonObject& obj);
    void handleRegister(ClientSession* from, const QJsonObject& obj);
    void handleFriendList(ClientSession* from);
    void handleSearchUser(ClientSession* from, const QJsonObject& obj);
    void handleFriendAdd(ClientSession* from, const QJsonObject& obj);
    void handleFriendReqList(ClientSession* from);
    void handleFriendHandle(ClientSession* from, const QJsonObject& obj);
    void handleChat(ClientSession* from, const QJsonObject& obj);
    void handleOfflineAck(ClientSession* from, const QJsonObject& obj);
    void handleSetNickname(ClientSession* from, const QJsonObject& obj);
    void handleSetAvatar(ClientSession* from, const QJsonObject& obj);
    void handleGetAvatar(ClientSession* from, const QJsonObject& obj);
    // 给刚上线的用户补发离线消息，一次最多一批
    void sendOfflineMessages(ClientSession* to);

    void notifyFriendsStatus(const QString& username, bool online);
    void notifyFriendListChanged(const QString& username);
    void notifyFriendsProfileChanged(const QString& username);

    QTcpServer* server;
    QList<ClientSession*> sessions;
    Database* db;
    QHash<QString, ClientSession*> onlineUsers;
};