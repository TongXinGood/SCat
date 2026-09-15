#include "../include/ChatStorage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <QDebug>

ChatStorage::ChatStorage(QObject* parent)
    : QObject(parent)
{
}

ChatStorage::~ChatStorage()
{
    close();
}

QString ChatStorage::dbPathFor(const QString& user)
{
    // Windows 上是 C:/Users/你/AppData/Roaming/SCat/SCat-Client/
    // 一个账号一个子目录，本机开两个客户端测试时读写的是不同文件，
    // 不会抢 SQLite 的文件锁
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base + "/" + user + "/chat.db";
}

bool ChatStorage::open(const QString& user)
{
    close();

    QString path = dbPathFor(user);

    // 目录不存在 SQLite 不会自己建，得先 mkpath
    QDir().mkpath(QFileInfo(path).absolutePath());

    // 起个连接名，不然会占用 QSqlDatabase 的默认连接
    connName = "chat_" + user;

    db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(path);

    if (!db.open()) {
        qDebug() << "chat db open failed:" << db.lastError().text();
        return false;
    }

    // WAL 模式：读和写不互相阻塞，程序异常退出时文件也不容易损坏
    QSqlQuery(db).exec("PRAGMA journal_mode=WAL");

    if (!createTables())
        return false;

    owner = user;
    qDebug() << "chat db opened:" << path;
    return true;
}

void ChatStorage::close()
{
    if (db.isOpen())
        db.close();

    // 必须先把成员置空，否则 removeDatabase 会报
    // "connection is still in use" 的警告
    db = QSqlDatabase();

    if (!connName.isEmpty()) {
        QSqlDatabase::removeDatabase(connName);
        connName.clear();
    }

    owner.clear();
}

bool ChatStorage::createTables()
{
    QSqlQuery q(db);

    // 注意：from 和 to 都是 SQL 关键字，不能直接当列名，
    // 所以这里叫 sender / receiver。
    // peer 这一列是冗余的（能从 sender/receiver 推出来），
    // 但存一份查询就只要一个条件，跟 friends 表存双向两行是一个道理
    if (!q.exec("CREATE TABLE IF NOT EXISTS messages ("
        "id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "msgid    TEXT    NOT NULL UNIQUE,"
        "peer     TEXT    NOT NULL,"
        "sender   TEXT    NOT NULL,"
        "receiver TEXT    NOT NULL,"
        "content  TEXT    NOT NULL,"
        "time     INTEGER NOT NULL,"
        "is_self  INTEGER NOT NULL)")) {
        qDebug() << "create table failed:" << q.lastError().text();
        return false;
    }

    // 查会话记录时按 peer 过滤、按 time 排序，这个索引让它不用全表扫描
    if (!q.exec("CREATE INDEX IF NOT EXISTS idx_peer_time ON messages(peer, time)")) {
        qDebug() << "create index failed:" << q.lastError().text();
        return false;
    }

    return true;
}

void ChatStorage::addMessage(const ChatMessage& msg)
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);

    // OR IGNORE：msgid 上有 UNIQUE 约束，重复的消息插不进去会被静默跳过。
    // 第 5 步离线消息重复投递时，去重全靠这一句
    q.prepare("INSERT OR IGNORE INTO messages "
        "(msgid, peer, sender, receiver, content, time, is_self) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(msg.msgid);
    q.addBindValue(msg.peer());
    q.addBindValue(msg.from);
    q.addBindValue(msg.to);
    q.addBindValue(msg.content);
    q.addBindValue(msg.time);
    q.addBindValue(msg.isSelf ? 1 : 0);

    if (!q.exec())
        qDebug() << "addMessage failed:" << q.lastError().text();
}

QList<ChatMessage> ChatStorage::loadHistory(const QString& peer, int limit) const
{
    QList<ChatMessage> list;
    if (!db.isOpen())
        return list;

    QSqlQuery q(db);

    // 先倒序取最近的 limit 条。如果写成正序 LIMIT，
    // 拿到的是最老的那几条，不是我们要的
    q.prepare("SELECT msgid, sender, receiver, content, time, is_self "
        "FROM messages WHERE peer = ? "
        "ORDER BY time DESC, id DESC LIMIT ?");
    q.addBindValue(peer);
    q.addBindValue(limit);

    if (!q.exec()) {
        qDebug() << "loadHistory failed:" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        ChatMessage msg;
        msg.msgid = q.value(0).toString();
        msg.from = q.value(1).toString();
        msg.to = q.value(2).toString();
        msg.content = q.value(3).toString();
        msg.time = q.value(4).toLongLong();
        msg.isSelf = q.value(5).toInt() != 0;

        // 倒着查出来的，往前插正好还原成时间正序
        list.prepend(msg);
    }

    return list;
}

bool ChatStorage::lastMessage(const QString& peer, ChatMessage& out) const
{
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("SELECT msgid, sender, receiver, content, time, is_self "
        "FROM messages WHERE peer = ? "
        "ORDER BY time DESC, id DESC LIMIT 1");
    q.addBindValue(peer);

    if (!q.exec() || !q.next())
        return false;

    out.msgid = q.value(0).toString();
    out.from = q.value(1).toString();
    out.to = q.value(2).toString();
    out.content = q.value(3).toString();
    out.time = q.value(4).toLongLong();
    out.isSelf = q.value(5).toInt() != 0;

    return true;
}

void ChatStorage::clearHistory(const QString& peer)
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);
    q.prepare("DELETE FROM messages WHERE peer = ?");
    q.addBindValue(peer);
    q.exec();
}

void ChatStorage::clearAll()
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);
    q.exec("DELETE FROM messages");

    // DELETE 只是把空间标记成可复用，文件大小纹丝不动。
    // VACUUM 会重建整个文件，才是真正把空间还给系统
    q.exec("VACUUM");
}