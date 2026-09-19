#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include "../../Protocol.h"

class NetWorkManager :public QObject
{
	Q_OBJECT
public:
	explicit NetWorkManager(QObject* parent = nullptr);
	void connectToServer(const QString& host, quint16 port);
	void disconnectFromServer();
	bool isConnected() const;

    void sendPacket(quint16 type, const QJsonObject& obj);

    qint64 pendingBytes() const;
signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& msg);
    void packetReceived(quint16 type, const QJsonObject& obj);
    void bytesWritten(qint64 bytes);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError err);
    void onReadyRead();
private:
    QTcpSocket* socket;
    QByteArray buffer;
};
#endif