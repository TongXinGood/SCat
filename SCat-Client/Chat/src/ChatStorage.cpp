#include "../include/ChatStorage.h"
#include <QDebug>

ChatStorage::ChatStorage(QObject* parent)
    : QObject(parent)
{
}

void ChatStorage::open(const QString& user)
{
    // 换账号了就把上一个账号的记录丢掉，免得串号
    if (owner != user)
        history.clear();

    owner = user;
    qDebug() << "chat storage opened for" << owner;
}

void ChatStorage::close()
{
    history.clear();
    owner.clear();
}

void ChatStorage::addMessage(const ChatMessage& msg)
{
    if (owner.isEmpty())
        return;

    // 不管是我发的还是收到的，都归到"跟对方的那段对话"里
    history[msg.peer()].append(msg);
}

QList<ChatMessage> ChatStorage::loadHistory(const QString& peer) const
{
    return history.value(peer);
}