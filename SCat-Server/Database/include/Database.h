#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QList>
#include <QDebug>
#include "../../Protocol.h"

struct FriendInfo
{
    QString username;
    QString nickname;
    QString avatar;
};

// 一条暂存的离线消息
struct OfflineMsg
{
    QString msgid;
    QString sender;
    QString receiver;
    QString content;
    qint64  time = 0;
};

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject* parent = nullptr);
    ~Database();
    bool connect(const QString& host, quint16 port,const QString& dbName,const QString& user, const QString& pwd);
    // 用户相关
    bool userExists(const QString& username);
    int  addUser(const QString& username, const QString& password,const QString& nickname);
    int  checkLogin(const QString& username, const QString& password);
    bool getUserInfo(const QString& username,QString& nickname, QString& avatar);
    // 按账号精确查一个人的公开信息（FriendInfo 里正好就是这三个字段）
    bool findUser(const QString& username, FriendInfo& out);
    bool getFriendList(const QString& username, QList<FriendInfo>& list);
    bool isFriend(const QString& a, const QString& b);
    bool addFriendRequest(const QString& sender, const QString& receiver);
    bool getPendingRequests(const QString& receiver, QList<FriendInfo>& list);
    int  handleRequest(const QString& receiver, const QString& sender, int action);

    // 离线消息
    bool addOfflineMsg(const QString& msgid, const QString& sender,const QString& receiver, const QString& content, qint64 time);
    bool getOfflineMsgs(const QString& receiver, QList<OfflineMsg>& list, int limit);
    bool deleteOfflineMsgs(const QString& receiver, const QStringList& msgids);
private:
    QSqlDatabase db;
};