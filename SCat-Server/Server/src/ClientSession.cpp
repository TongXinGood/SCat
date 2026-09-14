#include "../include/ClientSession.h"
#include <QDataStream>
#include <QJsonDocument>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket* sock, QObject* parent)
    : QObject(parent), socket(sock)
{
    socket->setParent(this);      // 接管生命周期

    connect(socket, &QTcpSocket::readyRead,
        this, &ClientSession::onReadyRead);
    connect(socket, &QTcpSocket::disconnected,
        this, &ClientSession::onDisconnected);
}
void ClientSession::setUser(const QString& name)
{
    userName = name;
}

QString ClientSession::peerInfo() const
{
    return socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());
}

void ClientSession::close()
{
    socket->disconnectFromHost();
}

void ClientSession::sendPacket(quint16 type, const QJsonObject& obj)
{
    if (socket->state() != QAbstractSocket::ConnectedState)
        return;

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << quint32(body.size()) << quint16(type);
    packet.append(body);

    socket->write(packet);
}

void ClientSession::onReadyRead()
{
    buffer.append(socket->readAll());

    while (true) {
        // 1. 头部够不够？
        if (buffer.size() < HEADER_SIZE)
            return;

        // 2. 读出长度和类型（只读不删）
        QDataStream in(buffer);
        in.setByteOrder(QDataStream::BigEndian);
        quint32 bodyLen = 0;
        quint16 type = 0;
        in >> bodyLen >> type;

        // 3. 长度合法性检查
        if (bodyLen > MAX_BODY_SIZE) {
            qDebug() << "invalid body size:" << bodyLen << "from" << peerInfo();
            socket->abort();
            return;
        }

        // 4. 整包到齐没有？
        if (buffer.size() < HEADER_SIZE + qint64(bodyLen))
            return;

        // 5. 切出这一包，从缓冲区移除
        QByteArray body = buffer.mid(HEADER_SIZE, bodyLen);
        buffer.remove(0, HEADER_SIZE + bodyLen);

        // 6. 解析 JSON
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "bad json from" << peerInfo() << ":" << err.errorString();
            continue;      // 这包丢掉，继续处理下一包
        }

        emit packetReceived(this, type, doc.object());
        // 回到循环开头，看缓冲区里还有没有下一包
    }
}

void ClientSession::onDisconnected()
{
    qDebug() << "client disconnected:" << peerInfo();
    emit closed(this);
}