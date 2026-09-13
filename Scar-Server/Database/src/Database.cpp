#include "../include/Database.h"

Database::Database(QObject* parent)
    : QObject(parent)
{
}

Database::~Database()
{
    if (db.isOpen())
        db.close();
}

bool Database::connect(const QString& host, quint16 port,
    const QString& dbName,
    const QString& user, const QString& pwd)
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(pwd);

    if (!db.open()) {
        qDebug() << "database open failed:" << db.lastError().text();
        return false;
    }

    qDebug() << "database connected:" << dbName;
    return true;
}

bool Database::userExists(const QString& username)
{
    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM users WHERE username = ?");
    q.addBindValue(username);

    if (!q.exec()) {
        qDebug() << "userExists failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

int Database::addUser(const QString& username, const QString& password,
    const QString& nickname)
{
    QSqlQuery q(db);
    q.prepare("INSERT INTO users (username, password, nickname) "
        "VALUES (?, ?, ?)");
    q.addBindValue(username);
    q.addBindValue(password);
    q.addBindValue(nickname);

    if (!q.exec()) {
        // 1062 = 违反唯一约束，说明用户名已存在
        if (q.lastError().nativeErrorCode() == "1062")
            return ERR_USER_EXIST;

        qDebug() << "addUser failed:" << q.lastError().text();
        return ERR_DB_ERROR;
    }

    qDebug() << "user registered:" << username;
    return ERR_OK;
}

int Database::checkLogin(const QString& username, const QString& password)
{
    QSqlQuery q(db);
    q.prepare("SELECT password FROM users WHERE username = ?");
    q.addBindValue(username);

    if (!q.exec()) {
        qDebug() << "checkLogin failed:" << q.lastError().text();
        return ERR_DB_ERROR;
    }

    if (!q.next())
        return ERR_USER_NOT_FOUND;

    // TODO: 密码目前是明文比对，以后改成 hash
    if (q.value(0).toString() != password)
        return ERR_WRONG_PASSWORD;

    return ERR_OK;
}

bool Database::getUserInfo(const QString& username,
    QString& nickname, QString& avatar)
{
    QSqlQuery q(db);
    q.prepare("SELECT nickname, avatar FROM users WHERE username = ?");
    q.addBindValue(username);

    if (!q.exec() || !q.next())
        return false;

    nickname = q.value(0).toString();
    avatar = q.value(1).toString();
    return true;
}