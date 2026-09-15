#ifndef FRIENDLISTITEM_H
#define FRIENDLISTITEM_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QString>

class FriendListItem : public QWidget
{
    Q_OBJECT
public:
    explicit FriendListItem(const QString& id, const QPixmap& avatar, const QString& name,
        const QString& lastMsg, QWidget* parent = nullptr);
    ~FriendListItem();

    QString getFriendId() const;
    QString getFriendName() const;

    void setLastMessage(const QString& msg);
    void setAvatar(const QPixmap& avatar);
    void setUnread(int count);                  // 0 = 不显示红点

private:
    void initUI(const QPixmap& avatar, const QString& name, const QString& lastMsg);

private:
    QString friendId;
    QString friendName;

    QLabel* lbAvatar;
    QLabel* lbName;
    QLabel* lbLastMsg;
    QLabel* lbUnread;           // 最右侧的未读数红点

    QHBoxLayout* mainLayout;
    QVBoxLayout* textLayout;
};

#endif // FRIENDLISTITEM_H