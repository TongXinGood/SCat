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
    // 头像直接传裁好的圆形 QPixmap，不再传路径 ——
    // 缩放、裁圆、默认图兜底统一在 AvatarUtils 里做
    explicit FriendListItem(const QString& id, const QPixmap& avatar, const QString& name,
        const QString& lastMsg, QWidget* parent = nullptr);
    ~FriendListItem();

    QString getFriendId() const;
    QString getFriendName() const;

    void setLastMessage(const QString& msg);
    void setAvatar(const QPixmap& avatar);      // 头像下载完之后刷新这一行

private:
    void initUI(const QPixmap& avatar, const QString& name, const QString& lastMsg);

private:
    QString friendId;
    QString friendName;

    QLabel* lbAvatar;
    QLabel* lbName;
    QLabel* lbLastMsg;

    QHBoxLayout* mainLayout;
    QVBoxLayout* textLayout;
};

#endif // FRIENDLISTITEM_H