#ifndef CHATNETWORK_H
#define CHATNETWORK_H

#include <QObject>
#include <QJsonObject>
#include "ChatMessage.h"
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"

class NetWorkManager;

// 聊天模块的逻辑层，只管收发包，不碰界面
class ChatNetWork : public QObject
{
    Q_OBJECT

public:
    explicit ChatNetWork(NetWorkManager* net, QObject* parent = nullptr);

public slots:
    void sendTextMessage(const QString& to, const QString& content);

signals:
    // 自己发的消息，服务端确认了（带回服务端时间戳）
    void messageSent(const ChatMessage& msg);
    // 收到别人发来的消息
    void messageReceived(const ChatMessage& msg);
    void sendFailed(const QString& reason);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleChatResp(const QJsonObject& obj);
    void handleChatPush(const QJsonObject& obj);
    void handleOfflinePush(const QJsonObject& obj);
private:
    NetWorkManager* net;
};

#endif // CHATNETWORK_H