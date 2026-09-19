#pragma once

#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>
#include <QHash>
#include <QUuid>
#include <QFile>


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

    // 文件上传
    void handleFileBegin(ClientSession* from, const QJsonObject& obj);
    void handleFileChunk(ClientSession* from, const QJsonObject& obj);
    void handleFileEnd(ClientSession* from, const QJsonObject& obj);
    void handleFileCancel(ClientSession* from, const QJsonObject& obj);
    void abortUpload(const QString& fileId);

    // 给刚上线的用户补发离线消息，一次最多一批
    void sendOfflineMessages(ClientSession* to);

    void notifyFriendsStatus(const QString& username, bool online);
    void notifyFriendListChanged(const QString& username);
    void notifyFriendsProfileChanged(const QString& username);

        // 一个正在接收中的上传。文件句柄全程开着，收一块写一块，
        // 绝不把整个文件攒在内存里 —— 500MB 攒不起
        struct Upload
    {
        QFile* file = nullptr;
        ClientSession* from = nullptr;   // 只认发起上传的那条连接
        QString receiver;
        QString fileName;
        qint64  fileSize = 0;            // 客户端声明的大小
        qint64  received = 0;            // 实际收到多少，收完要核对
    };


    QTcpServer* server;
    QList<ClientSession*> sessions;
    Database* db;
    QHash<QString, ClientSession*> onlineUsers;
    QHash<QString, Upload> uploads;
};