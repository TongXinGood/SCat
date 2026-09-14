#include "../include/Server.h"
#include "../include/ClientSession.h"
#include "../../Database/include/Database.h"
#include <QDebug>

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
    }
    sessions.removeOne(session);
    session->deleteLater();

    qDebug() << "session removed | online users:" << onlineUsers.size()<< "| connections:" << sessions.size();
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
}

void Server::handleRegister(ClientSession* from, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();
    QString nickname = obj["nickname"].toString().trimmed();

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