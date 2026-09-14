#include "../include/Server.h"
#include "../include/ClientSession.h"
#include "../../Database/include/Database.h"
#include <QDebug>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>

void Server::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket* sock = server->nextPendingConnection();

        ClientSession* session = new ClientSession(sock, this);
        sessions.append(session);

        connect(session, &ClientSession::packetReceived,
            this, &Server::onPacketReceived);
        connect(session, &ClientSession::closed,
            this, &Server::onSessionClosed);

        qDebug() << "new client:" << session->peerInfo()
            << "| online:" << sessions.size();
    }
}

void Server::onPacketReceived(ClientSession* from, quint16 type, const QJsonObject& obj)
{
    qDebug() << "recv from" << from->peerInfo() << "type" << type;

    switch (type) {
    case MSG_HEARTBEAT:
        from->sendPacket(MSG_HEARTBEAT_RESP, QJsonObject());
        break;

    case MSG_LOGIN_REQ:
        handleLogin(from, obj);
        break;

    case MSG_REG_REQ:
        handleRegister(from, obj);
        break;

    case MSG_FRIEND_LIST_REQ:
        handleFriendList(from);
        break;

    case MSG_CHAT_REQ:
        handleChat(from, obj);
        break;

    default:
        qDebug() << "unknown type:" << type;
        break;
    }
}

void Server::onSessionClosed(ClientSession* session)
{
    if (session->isLogined()) {
        // 只有在线表里记的确实是这条连接时才移除。
        // 防止以后改成"顶号"策略时，旧连接断开把新连接误删掉。
        if (onlineUsers.value(session->username()) == session)
            onlineUsers.remove(session->username());

        qDebug() << "user offline:" << session->username();

        // 注意顺序：先从在线表移除再通知，否则会给自己也推一条
        notifyFriendsStatus(session->username(), false);
    }

    sessions.removeOne(session);
    session->deleteLater();

    qDebug() << "session removed | online users:" << onlineUsers.size()
        << "| connections:" << sessions.size();
}

Server::Server(Database* db, QObject* parent)
    : QObject(parent), db(db)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection,
        this, &Server::onNewConnection);
}

bool Server::start(quint16 port)
{
    if (!server->listen(QHostAddress::Any, port)) {
        qDebug() << "listen failed:" << server->errorString();
        return false;
    }
    qDebug() << "server listening on port" << port;
    return true;
}

void Server::handleLogin(ClientSession* from, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();

    QJsonObject resp;

    // 同一条连接重复发登录包，直接拒绝，避免在线表里出现脏数据
    if (from->isLogined()) {
        qDebug() << "duplicate login on same connection:" << from->peerInfo();
        resp["code"] = ERR_ALREADY_ONLINE;
        from->sendPacket(MSG_LOGIN_RESP, resp);
        return;
    }

    int code = db->checkLogin(username, password);

    // 账号密码都没问题，再查在线表：已经在别处登录就拒绝这次登录
    if (code == ERR_OK && onlineUsers.contains(username))
        code = ERR_ALREADY_ONLINE;

    resp["code"] = code;

    if (code == ERR_OK) {
        QString nickname, avatar;
        db->getUserInfo(username, nickname, avatar);

        // 绑定：这条连接 = 这个用户。后面转发聊天消息全靠它
        from->setUser(username);
        onlineUsers.insert(username, from);

        resp["username"] = username;
        resp["nickname"] = nickname;
        resp["avatar"] = avatar;

        qDebug() << "login success:" << username
            << "| online users:" << onlineUsers.size();
    }
    else {
        qDebug() << "login failed:" << username << "code:" << code;
    }

    from->sendPacket(MSG_LOGIN_RESP, resp);

    if (code == ERR_OK)
        notifyFriendsStatus(from->username(), true);
}

void Server::handleRegister(ClientSession* from, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();
    QString nickname = obj["nickname"].toString().trimmed();

    static const QRegularExpression reName("^[a-zA-Z0-9]{4,15}$");
    if (!reName.match(username).hasMatch()) {
        QJsonObject resp;
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_REG_RESP, resp);
        qDebug() << "register rejected, bad username:" << username;
        return;
    }

    // 密码长度也查一下，跟客户端的规则保持一致
    if (password.length() < 8 || password.length() > 16) {
        QJsonObject resp;
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_REG_RESP, resp);
        qDebug() << "register rejected, bad password length:" << username;
        return;
    }

    if (nickname.isEmpty())
        nickname = username;

    int code = db->addUser(username, password, nickname);

    QJsonObject resp;
    resp["code"] = code;

    if (code == ERR_OK)
        qDebug() << "register success:" << username << nickname;
    else
        qDebug() << "register failed:" << username << "code:" << code;

    from->sendPacket(MSG_REG_RESP, resp);
}

void Server::handleFriendList(ClientSession* from)
{
    QJsonObject resp;

    // 没登录的连接不给查。这就是第 1 步把身份绑到连接上的好处：
    // 客户端不用传自己是谁，也伪造不了别人的身份
    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FRIEND_LIST_RESP, resp);
        qDebug() << "friend list rejected, not logged in:" << from->peerInfo();
        return;
    }

    QList<FriendInfo> list;
    if (!db->getFriendList(from->username(), list)) {
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_FRIEND_LIST_RESP, resp);
        return;
    }

    QJsonArray arr;
    for (const FriendInfo& info : list) {
        QJsonObject obj;
        obj["username"] = info.username;
        obj["nickname"] = info.nickname;
        obj["avatar"] = info.avatar;
        // 在线状态直接查内存里的在线表，不用碰数据库
        obj["online"] = onlineUsers.contains(info.username);
        arr.append(obj);
    }

    resp["code"] = ERR_OK;
    resp["friends"] = arr;

    from->sendPacket(MSG_FRIEND_LIST_RESP, resp);

    qDebug() << "friend list sent to" << from->username()
        << ":" << arr.size() << "friends";
}

void Server::notifyFriendsStatus(const QString& username, bool online)
{
    QList<FriendInfo> list;
    if (!db->getFriendList(username, list))
        return;

    QJsonObject push;
    push["username"] = username;
    push["online"] = online;

    int sent = 0;
    for (const FriendInfo& info : list) {
        // 只推给当前在线的好友。不在线的不用管，
        // 他下次登录时拉好友列表就能拿到最新状态
        ClientSession* session = onlineUsers.value(info.username, nullptr);
        if (session) {
            session->sendPacket(MSG_FRIEND_STATUS_PUSH, push);
            ++sent;
        }
    }

    qDebug() << "status push:" << username << (online ? "online" : "offline")
        << "->" << sent << "friends";
}

void Server::handleChat(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_CHAT_RESP, resp);
        return;
    }

    QString to = obj["to"].toString();
    QString content = obj["content"].toString();

    if (to.isEmpty() || content.isEmpty()) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_CHAT_RESP, resp);
        return;
    }

    // 时间戳统一由服务端生成。两台电脑的系统时间可能对不上，
    // 各自取本地时间会导致同一段对话在两边的顺序不一样
    qint64 time = QDateTime::currentMSecsSinceEpoch();

    // 查在线表，找到对方的连接就推过去
    ClientSession* target = onlineUsers.value(to, nullptr);
    if (target) {
        QJsonObject push;
        push["from"] = from->username();
        push["to"] = to;
        push["content"] = content;
        push["time"] = time;

        target->sendPacket(MSG_CHAT_PUSH, push);

        qDebug() << "chat" << from->username() << "->" << to
            << ":" << content.left(20);
    }
    else {
        // 对方不在线，这条消息暂时丢掉。
        // 第 5 步会改成存进 offline_msg 表，等他上线再补发
        qDebug() << "chat target offline, dropped:" << to;
    }

    // 回执给发送方，带上服务端时间戳，让它拿去存本地
    resp["code"] = ERR_OK;
    resp["to"] = to;
    resp["content"] = content;
    resp["time"] = time;
    resp["delivered"] = (target != nullptr);

    from->sendPacket(MSG_CHAT_RESP, resp);
}