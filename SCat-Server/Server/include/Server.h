#pragma once

#include <QObject>
#include <QTcpServer>
#include <QList>
#include <QJsonObject>

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


private:
    void handleLogin(ClientSession* from, const QJsonObject& obj);
    void handleRegister(ClientSession* from, const QJsonObject& obj);
    QTcpServer* server;
    QList<ClientSession*> sessions;
    Database* db;
};