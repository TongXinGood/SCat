#include "../include/ChatStorage.h"
#include "../../Other/include/AppPath.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <QMap>
#include <QStringList>
#include <QDebug>

ChatStorage::ChatStorage(QObject* parent)
    : QObject(parent)
{
}

ChatStorage::~ChatStorage()
{
    close();
}



bool ChatStorage::open(const QString& user)
{
    close();

    // 路径和建目录都交给 AppPath
    QString path = AppPath::chatDbPath(user);

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

    if (!migrateTables())
        return false;

    resetPendingTransfers();

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
        "is_self  INTEGER NOT NULL,"
        "kind     INTEGER NOT NULL DEFAULT 0,"
        "img_name TEXT,"
        "img_w    INTEGER NOT NULL DEFAULT 0,"
        "img_h    INTEGER NOT NULL DEFAULT 0,"
        "file_id   TEXT,"
        "file_name TEXT,"
        "file_size INTEGER NOT NULL DEFAULT 0,"
        "file_path TEXT,"
        "file_state INTEGER NOT NULL DEFAULT 0)")) {
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

bool ChatStorage::migrateTables()
{
    QSqlQuery q(db);

    // 先查现在表里到底有哪些列。SQLite 没有 "ADD COLUMN IF NOT EXISTS"，
    // 重复加同名列会直接报错，所以只能自己比对
    if (!q.exec("PRAGMA table_info(messages)")) {
        qDebug() << "read table info failed:" << q.lastError().text();
        return false;
    }

    QStringList existing;
    while (q.next())
        existing << q.value(1).toString();      // 结果集第 1 列就是列名

    // 列名 -> 建列语句。以后再加新列，往这里补一行就行，
    // 老库新库都能自动对齐
    QMap<QString, QString> columns;
    columns["kind"] = "kind INTEGER NOT NULL DEFAULT 0";
    columns["img_name"] = "img_name TEXT";
    columns["img_w"] = "img_w INTEGER NOT NULL DEFAULT 0";
    columns["img_h"] = "img_h INTEGER NOT NULL DEFAULT 0";
    columns["file_id"] = "file_id TEXT";
    columns["file_name"] = "file_name TEXT";
    columns["file_size"] = "file_size INTEGER NOT NULL DEFAULT 0";
    columns["file_path"] = "file_path TEXT";
    columns["file_state"] = "file_state INTEGER NOT NULL DEFAULT 0";

    for (auto it = columns.constBegin(); it != columns.constEnd(); ++it) {
        if (existing.contains(it.key()))
            continue;

        // SQLite 的 ADD COLUMN 只改表结构不动数据，几万条记录也是瞬间完成
        if (!q.exec("ALTER TABLE messages ADD COLUMN " + it.value())) {
            qDebug() << "add column failed:" << it.key()
                << q.lastError().text();
            return false;
        }

        qDebug() << "messages table migrated, column added:" << it.key();
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
        "(msgid, peer, sender, receiver, content, time, is_self, "
        "kind, img_name, img_w, img_h, "
        "file_id, file_name, file_size, file_path, file_state) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(msg.msgid);
    q.addBindValue(msg.peer());
    q.addBindValue(msg.from);
    q.addBindValue(msg.to);
    q.addBindValue(msg.content);
    q.addBindValue(msg.time);
    q.addBindValue(msg.isSelf ? 1 : 0);
    q.addBindValue(msg.kind);
    q.addBindValue(msg.imgName);
    q.addBindValue(msg.imgW);
    q.addBindValue(msg.imgH);
    q.addBindValue(msg.fileId);
    q.addBindValue(msg.fileName);
    q.addBindValue(msg.fileSize);
    q.addBindValue(msg.filePath);
    q.addBindValue(msg.fileState);

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
    q.prepare("SELECT msgid, sender, receiver, content, time, is_self, "
        "kind, img_name, img_w, img_h, "
        "file_id, file_name, file_size, file_path, file_state "
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
        msg.kind = q.value(6).toInt();
        msg.imgName = q.value(7).toString();
        msg.imgW = q.value(8).toInt();
        msg.imgH = q.value(9).toInt();
        msg.fileId = q.value(10).toString();
        msg.fileName = q.value(11).toString();
        msg.fileSize = q.value(12).toLongLong();
        msg.filePath = q.value(13).toString();
        msg.fileState = q.value(14).toInt();

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
    q.prepare("SELECT msgid, sender, receiver, content, time, is_self, "
        "kind, img_name, img_w, img_h, "
        "file_id, file_name, file_size, file_path, file_state "
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
    out.kind = q.value(6).toInt();
    out.imgName = q.value(7).toString();
    out.imgW = q.value(8).toInt();
    out.imgH = q.value(9).toInt();
    out.fileId = q.value(10).toString();
    out.fileName = q.value(11).toString();
    out.fileSize = q.value(12).toLongLong();
    out.filePath = q.value(13).toString();
    out.fileState = q.value(14).toInt();
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

void ChatStorage::updateFileState(const QString& msgid, int state,
    const QString& fileId)
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);

    // fileId 为空时不要去覆盖已有的值 —— 失败重试、取消这些场景
    // 都不带 fileId，写空了下次就找不到服务端那个文件了
    if (fileId.isEmpty()) {
        q.prepare("UPDATE messages SET file_state = ? WHERE msgid = ?");
        q.addBindValue(state);
        q.addBindValue(msgid);
    }
    else {
        q.prepare("UPDATE messages SET file_state = ?, file_id = ? WHERE msgid = ?");
        q.addBindValue(state);
        q.addBindValue(fileId);
        q.addBindValue(msgid);
    }

    if (!q.exec())
        qDebug() << "updateFileState failed:" << q.lastError().text();
}

void ChatStorage::updateFilePath(const QString& msgid, const QString& filePath)
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);
    q.prepare("UPDATE messages SET file_path = ? WHERE msgid = ?");
    q.addBindValue(filePath);
    q.addBindValue(msgid);

    if (!q.exec())
        qDebug() << "updateFilePath failed:" << q.lastError().text();
}

bool ChatStorage::messageById(const QString& msgid, ChatMessage& out) const
{
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("SELECT msgid, sender, receiver, content, time, is_self, "
        "kind, img_name, img_w, img_h, "
        "file_id, file_name, file_size, file_path, file_state "
        "FROM messages WHERE msgid = ?");
    q.addBindValue(msgid);

    if (!q.exec() || !q.next())
        return false;

    out.msgid = q.value(0).toString();
    out.from = q.value(1).toString();
    out.to = q.value(2).toString();
    out.content = q.value(3).toString();
    out.time = q.value(4).toLongLong();
    out.isSelf = q.value(5).toInt() != 0;
    out.kind = q.value(6).toInt();
    out.imgName = q.value(7).toString();
    out.imgW = q.value(8).toInt();
    out.imgH = q.value(9).toInt();
    out.fileId = q.value(10).toString();
    out.fileName = q.value(11).toString();
    out.fileSize = q.value(12).toLongLong();
    out.filePath = q.value(13).toString();
    out.fileState = q.value(14).toInt();

    return true;
}

void ChatStorage::resetPendingTransfers()
{
    if (!db.isOpen())
        return;

    QSqlQuery q(db);

    // 自己发的传一半没了 -> 失败。源文件还在自己盘上，点重试从头传
    q.prepare("UPDATE messages SET file_state = ? "
        "WHERE file_state = ? AND is_self = 1");
    q.addBindValue(FILE_STATE_FAILED);
    q.addBindValue(FILE_STATE_SENDING);

    if (!q.exec())
        qDebug() << "reset sending failed:" << q.lastError().text();

    // 别人发来的下一半没了 -> 回到"等待下载"，不是失败。
    // 文件还在服务端躺着（3 天内），重新点一下就行
    q.prepare("UPDATE messages SET file_state = ? "
        "WHERE file_state = ? AND is_self = 0");
    q.addBindValue(FILE_STATE_READY);
    q.addBindValue(FILE_STATE_DOWNLOADING);

    if (!q.exec())
        qDebug() << "reset downloading failed:" << q.lastError().text();
}