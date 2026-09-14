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
    sessions.removeOne(session);
    session->deleteLater();
    qDebug() << "session removed, online:" << sessions.size();
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

    int code = db->checkLogin(username, password);

    QJsonObject resp;
    resp["code"] = code;

    if (code == ERR_OK) {
        QString nickname, avatar;
        db->getUserInfo(username, nickname, avatar);

        resp["username"] = username;
        resp["nickname"] = nickname;
        resp["avatar"] = avatar;

        qDebug() << "login success:" << username;
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