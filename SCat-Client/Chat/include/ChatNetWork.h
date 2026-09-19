#ifndef CHATNETWORK_H
#define CHATNETWORK_H

#include <QObject>
#include <QJsonObject>
#include <QQueue>
#include <QSize>
#include <QByteArray>
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
    void sendImageMessage(const QString& to, const QString& imgName,const QByteArray& data, const QSize& size);

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
    void fillImage(ChatMessage& msg, const QJsonObject& obj);
    // 文件消息的公共解析。只填元信息，不下载 ——
    // 500MB 自动下太狠了，要等用户点那个"下载"
    void fillFile(ChatMessage& msg, const QJsonObject& obj);
private:
    // 图片发出去之后，等回执时要知道这张图存在本地哪个文件里。
    // imgName 纯粹是本地信息，没必要塞进协议让服务端转一圈，
    // 在这排个队就够了 —— TCP 保证回执顺序跟请求顺序一致，先进先出正好对得上
    struct PendingImage
    {
        QString imgName;
        int w = 0;
        int h = 0;
    };
    NetWorkManager* net;
    QQueue<PendingImage> pendingImages;
};

#endif // CHATNETWORK_H