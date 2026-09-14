#ifndef CHATSTORAGE_H
#define CHATSTORAGE_H

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include "ChatMessage.h"

// 聊天记录的存取。
// 这一步先用内存（QHash）实现，目的是把聊天流程跑通；
// 下一步会把内部实现换成本地 SQLite，对外接口保持不变，
// 所以到时候其它文件一行都不用改。
class ChatStorage : public QObject
{
    Q_OBJECT

public:
    explicit ChatStorage(QObject* parent = nullptr);

    // 登录成功后调用，之后的读写都归这个账号。
    // 换账号登录时要重新 open，不能串号
    void open(const QString& user);
    void close();

    void addMessage(const ChatMessage& msg);
    QList<ChatMessage> loadHistory(const QString& peer) const;

private:
    QString owner;      // 当前登录的账号

    // 好友的 username -> 跟这个人的全部聊天记录
    QHash<QString, QList<ChatMessage>> history;
};

#endif // CHATSTORAGE_H