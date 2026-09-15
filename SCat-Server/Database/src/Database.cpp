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

bool Database::getFriendList(const QString& username, QList<FriendInfo>& list)
{
    list.clear();

    QSqlQuery q(db);
    // friends 表存的是双向两行，所以查"我的好友"只要一个条件就够了。
    // JOIN users 是为了把好友的昵称和头像一起带出来，省得再查一遍
    q.prepare("SELECT u.username, u.nickname, u.avatar "
        "FROM friends f "
        "JOIN users u ON f.friend = u.username "
        "WHERE f.username = ? "
        "ORDER BY u.nickname");
    q.addBindValue(username);

    if (!q.exec()) {
        qDebug() << "getFriendList failed:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        FriendInfo info;
        info.username = q.value(0).toString();
        info.nickname = q.value(1).toString();
        info.avatar = q.value(2).toString();
        list.append(info);
    }

    return true;
}

bool Database::addOfflineMsg(const QString& msgid, const QString& sender,
    const QString& receiver, const QString& content, qint64 time)
{
    QSqlQuery q(db);
    q.prepare("INSERT INTO offline_msg (msgid, sender, receiver, content, send_time) "
        "VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(msgid);
    q.addBindValue(sender);
    q.addBindValue(receiver);
    q.addBindValue(content);
    q.addBindValue(time);

    if (!q.exec()) {
        qDebug() << "addOfflineMsg failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool Database::getOfflineMsgs(const QString& receiver, QList<OfflineMsg>& list, int limit)
{
    list.clear();

    QSqlQuery q(db);
    // 按 id 正序取，保证补发的顺序跟当初发送的顺序一致
    q.prepare("SELECT msgid, sender, receiver, content, send_time "
        "FROM offline_msg WHERE receiver = ? ORDER BY id ASC LIMIT ?");
    q.addBindValue(receiver);
    q.addBindValue(limit);

    if (!q.exec()) {
        qDebug() << "getOfflineMsgs failed:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        OfflineMsg m;
        m.msgid = q.value(0).toString();
        m.sender = q.value(1).toString();
        m.receiver = q.value(2).toString();
        m.content = q.value(3).toString();
        m.time = q.value(4).toLongLong();
        list.append(m);
    }

    return true;
}

bool Database::deleteOfflineMsgs(const QString& receiver, const QStringList& msgids)
{
    if (msgids.isEmpty())
        return true;

    // IN (?, ?, ?...) 的占位符个数得按实际数量拼出来
    QStringList marks;
    for (int i = 0; i < msgids.size(); ++i)
        marks << "?";

    QSqlQuery q(db);
    // 带上 receiver 条件，确保只能删自己的那些，
    // 免得客户端传个别人的 msgid 就把别人的消息删了
    q.prepare("DELETE FROM offline_msg WHERE receiver = ? AND msgid IN ("
        + marks.join(",") + ")");
    q.addBindValue(receiver);
    for (const QString& id : msgids)
        q.addBindValue(id);

    if (!q.exec()) {
        qDebug() << "deleteOfflineMsgs failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool Database::findUser(const QString& username, FriendInfo& out)
{
    QSqlQuery q(db);
    q.prepare("SELECT username, nickname, avatar FROM users WHERE username = ?");
    q.addBindValue(username);

    if (!q.exec()) {
        qDebug() << "findUser failed:" << q.lastError().text();
        return false;
    }

    if (!q.next())
        return false;      // 没这个人，不是错误

    out.username = q.value(0).toString();
    out.nickname = q.value(1).toString();
    out.avatar = q.value(2).toString();
    return true;
}

bool Database::isFriend(const QString& a, const QString& b)
{
    QSqlQuery q(db);
    // friends 表存的是双向两行，所以查一个方向就够了
    q.prepare("SELECT 1 FROM friends WHERE username = ? AND friend = ?");
    q.addBindValue(a);
    q.addBindValue(b);

    if (!q.exec()) {
        qDebug() << "isFriend failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

bool Database::addFriendRequest(const QString& sender, const QString& receiver)
{
    QSqlQuery q(db);
    // uk_pair(sender, receiver) 保证一对人之间只有一条记录。
    // 不管之前是被拒过还是挂着没处理，这里都是把那一条重置成"待处理"，
    // 不会堆出一堆历史记录 —— 这也正是"关掉重开还能再申请"的实现
    q.prepare("INSERT INTO friend_request (sender, receiver, status) VALUES (?, ?, 0) "
        "ON DUPLICATE KEY UPDATE status = 0, "
        "created_at = CURRENT_TIMESTAMP, handled_at = NULL");
    q.addBindValue(sender);
    q.addBindValue(receiver);

    if (!q.exec()) {
        qDebug() << "addFriendRequest failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool Database::getPendingRequests(const QString& receiver, QList<FriendInfo>& list)
{
    list.clear();

    QSqlQuery q(db);
    // JOIN users 把申请人的昵称头像一起带出来，省得客户端再查一遍
    q.prepare("SELECT u.username, u.nickname, u.avatar "
        "FROM friend_request r "
        "JOIN users u ON r.sender = u.username "
        "WHERE r.receiver = ? AND r.status = 0 "
        "ORDER BY r.created_at DESC");
    q.addBindValue(receiver);

    if (!q.exec()) {
        qDebug() << "getPendingRequests failed:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        FriendInfo info;
        info.username = q.value(0).toString();
        info.nickname = q.value(1).toString();
        info.avatar = q.value(2).toString();
        list.append(info);
    }

    return true;
}

int Database::handleRequest(const QString& receiver, const QString& sender, int action)
{
    int status = (action == HANDLE_ACCEPT) ? 1 : 2;

    // 改申请状态和插好友关系必须一起成功，中间失败了要能退回去
    db.transaction();

    QSqlQuery q(db);
    // 只更新还处于"待处理"的那一条。如果它已经被处理过了
    // （比如两个窗口同时点了同意），numRowsAffected 会是 0
    q.prepare("UPDATE friend_request SET status = ?, handled_at = CURRENT_TIMESTAMP "
        "WHERE sender = ? AND receiver = ? AND status = 0");
    q.addBindValue(status);
    q.addBindValue(sender);
    q.addBindValue(receiver);

    if (!q.exec()) {
        qDebug() << "handleRequest failed:" << q.lastError().text();
        db.rollback();
        return ERR_DB_ERROR;
    }

    if (q.numRowsAffected() <= 0) {
        db.rollback();
        return ERR_REQUEST_NOT_FOUND;
    }

    if (action == HANDLE_ACCEPT) {
        // friends 存双向两行，一条语句插两行。
        // INSERT IGNORE 兜底，防止因为某些原因已经存在而报错
        QSqlQuery q2(db);
        q2.prepare("INSERT IGNORE INTO friends (username, friend) VALUES (?, ?), (?, ?)");
        q2.addBindValue(receiver);
        q2.addBindValue(sender);
        q2.addBindValue(sender);
        q2.addBindValue(receiver);

        if (!q2.exec()) {
            qDebug() << "insert friends failed:" << q2.lastError().text();
            db.rollback();
            return ERR_DB_ERROR;
        }

        // 如果对方之前也申请过我（两个人互相加），把那条反向记录一并标记成已同意，
        // 免得对方的铃铛里永远挂着一条处理不掉的申请
        QSqlQuery q3(db);
        q3.prepare("UPDATE friend_request SET status = 1, handled_at = CURRENT_TIMESTAMP "
            "WHERE sender = ? AND receiver = ? AND status = 0");
        q3.addBindValue(receiver);
        q3.addBindValue(sender);
        q3.exec();
    }

    db.commit();
    return ERR_OK;
}