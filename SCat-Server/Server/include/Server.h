#pragma once

#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>
#include <QHash>
#include <QUuid>
#include <QFile>
#include <QTimer>


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
    void onSessionBytesWritten(ClientSession* session);
    void onCleanTimer();

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
    void deliverFileMessage(const QString& sender, const QString& receiver,const QString& fileId, const QString& fileName, qint64 fileSize);
    // 文件下载
    void handleFilePull(ClientSession* from, const QJsonObject& obj);
    void sendFileChunks(const QString& fileId);   // 能塞多少塞多少，塞不下就等
    void abortDownload(const QString& fileId);
    void cleanExpiredFiles();    // 超过 FILE_KEEP_DAYS 天的，记录和文件一起删
    void cleanOrphanFiles();     // 磁盘上有、数据库里没有的（异常退出留下的）

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
        struct Download
        {
            QFile* file = nullptr;
            ClientSession* to = nullptr;
            QString fileName;
            qint64  fileSize = 0;
            qint64  sent = 0;
            int     seq = 0;
        };


    QTcpServer* server;
    QTimer* cleanTimer;
    QList<ClientSession*> sessions;
    Database* db;
    QHash<QString, ClientSession*> onlineUsers;
    QHash<QString, Upload> uploads;
    QHash<QString, Download> downloads;
};