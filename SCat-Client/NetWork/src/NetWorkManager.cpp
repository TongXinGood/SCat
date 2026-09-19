#include "../include/NetWorkManager.h"
#include <QDebug>

NetWorkManager::NetWorkManager(QObject* parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected,
        this, &NetWorkManager::onConnected);
    connect(socket, &QTcpSocket::disconnected,
        this, &NetWorkManager::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred,
        this, &NetWorkManager::onErrorOccurred);
    connect(socket, &QTcpSocket::readyRead,
        this, &NetWorkManager::onReadyRead);
    connect(socket, &QTcpSocket::bytesWritten,
        this, &NetWorkManager::bytesWritten);
}
qint64 NetWorkManager::pendingBytes() const
{
    return socket->bytesToWrite();
}

void NetWorkManager::connectToServer(const QString& host, quint16 port)
{
    // 已经连上或正在连，就不重复发起
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "already connecting or connected, state:" << socket->state();
        return;
    }

    qDebug() << "connecting to" << host << ":" << port;
    socket->connectToHost(host, port);
}

void NetWorkManager::disconnectFromServer()
{
    if (socket->state() == QAbstractSocket::UnconnectedState)
        return;

    socket->disconnectFromHost();
}

bool NetWorkManager::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void NetWorkManager::onConnected()
{
    qDebug() << "connected to server";
    emit connected();
}

void NetWorkManager::onDisconnected()
{
    buffer.clear();
    qDebug() << "disconnected from server";
    emit disconnected();
}

void NetWorkManager::onErrorOccurred(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err);
    qDebug() << "socket error:" << socket->errorString();
    emit errorOccurred(socket->errorString());
}

void NetWorkManager::sendPacket(quint16 type, const QJsonObject& obj)
{
    if (!isConnected()) {
        qDebug() << "sendPacket failed: not connected";
        return;
    }

    // 1. JSON -> 字节
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // 2. 拼 6 字节包头
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << quint32(body.size()) << quint16(type);

    // 3. 接上包体
    packet.append(body);

    // 4. 一次性发出
    socket->write(packet);

    qDebug() << "send type" << type << "body" << body.size() << "bytes";
}
void NetWorkManager::onReadyRead()
{
    buffer.append(socket->readAll());

    while (true) {
        if (buffer.size() < HEADER_SIZE)
            return;

        QDataStream in(buffer);
        in.setByteOrder(QDataStream::BigEndian);
        quint32 bodyLen = 0;
        quint16 type = 0;
        in >> bodyLen >> type;

        if (bodyLen > MAX_BODY_SIZE) {
            qDebug() << "invalid body size:" << bodyLen;
            socket->abort();
            return;
        }

        if (buffer.size() < HEADER_SIZE + qint64(bodyLen))
            return;

        QByteArray body = buffer.mid(HEADER_SIZE, bodyLen);
        buffer.remove(0, HEADER_SIZE + bodyLen);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "bad json:" << err.errorString();
            continue;
        }

        emit packetReceived(type, doc.object());
    }
}