#ifndef CHATSTORAGE_H
#define CHATSTORAGE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QSqlDatabase>
#include "ChatMessage.h"

// 聊天记录的本地存取，用 SQLite。
// 每个账号一个独立的 chat.db 文件，路径在
// C:/Users/你/AppData/Roaming/SCat/SCat-Client/<账号>/chat.db
class ChatStorage : public QObject
{
    Q_OBJECT

public:
    explicit ChatStorage(QObject* parent = nullptr);
    ~ChatStorage();

    // 登录成功后调用，打开这个账号的数据库
    bool open(const QString& user);
    void close();

    void addMessage(const ChatMessage& msg);

    // 取跟某个人的聊天记录，默认只取最近 200 条。
    // 一次全读出来的话，聊了几万条以后切会话会卡
    QList<ChatMessage> loadHistory(const QString& peer, int limit = 200) const;

    // 最后一条消息，用来填左边列表的副标题
    bool lastMessage(const QString& peer, ChatMessage& out) const;

    void clearHistory(const QString& peer);   // 清跟某个人的
    void clearAll();                          // 清全部

    // 数据库文件路径，调试时想手动删文件用得上
    static QString dbPathFor(const QString& user);

private:
    bool createTables();

private:
    QString owner;         // 当前登录的账号
    QSqlDatabase db;
    QString connName;      // 连接名，避免跟别处的 QSqlDatabase 抢默认连接
};

#endif // CHATSTORAGE_H