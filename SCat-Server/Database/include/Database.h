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

    bool getFriendList(const QString& username, QList<FriendInfo>& list);
private:
    QSqlDatabase db;
};