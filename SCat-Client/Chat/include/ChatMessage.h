#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QMetaType>

// 一条聊天消息。网络层、存储层、界面层都用这个结构传递
struct ChatMessage
{
    QString from;           // 发送方 username
    QString to;             // 接收方 username
    QString content;        // 文本内容
    qint64  time = 0;       // 服务端生成的毫秒时间戳
    bool    isSelf = false; // 是不是我发的，画气泡时决定摆左边还是右边

    // 这条消息属于跟谁的对话
    QString peer() const { return isSelf ? to : from; }
};

// 让 ChatMessage 能在信号槽里传递
Q_DECLARE_METATYPE(ChatMessage)

#endif // CHATMESSAGE_H