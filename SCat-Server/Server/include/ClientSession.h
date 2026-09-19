#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "../../Protocol.h"

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(QTcpSocket* sock, QObject* parent = nullptr);

    void sendPacket(quint16 type, const QJsonObject& obj);
    void close();

    QString peerInfo() const;      // 调试用，返回 "ip:port"
    void setUser(const QString& name);
    bool isLogined() const { return !userName.isEmpty(); }
    QString username() const { return userName; }

    qint64 pendingBytes() const;
signals:
    void packetReceived(ClientSession* from, quint16 type, const QJsonObject& obj);
    void closed(ClientSession* self);
    void bytesWritten(ClientSession* self);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* socket;
    QByteArray buffer;      // 这条连接私有的拆包缓冲
    

    QString userName;       // 空 = 未登录
};