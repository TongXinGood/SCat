#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>

class FriendList : public QWidget
{
    Q_OBJECT
public:
    explicit FriendList(QWidget* parent = nullptr);
    ~FriendList();

    void addFriendItem(const QString& id, const QPixmap& avatar,
        const QString& name, const QString& lastMsg);
    void clearFriends();
    void clearSelection();

    void updateLastMessage(const QString& id, const QString& msg);
    void updateAvatar(const QString& id, const QPixmap& avatar);
    void setUnread(const QString& id, int count);
    void selectFriend(const QString& id);        // 点通知时程序化选中某个好友

signals:
    void sendFriendSelected(const QString& friendId, const QString& friendName);
    void sendFriendUnselected();
    void sendAddFriendClicked();

private slots:
    void onSearchTextChanged(const QString& text);
    void onItemClicked(QListWidgetItem* item);
    void onAddClicked();

private:
    void initUI();
    void initConnect();

private:
    QVBoxLayout* mainLayout;

    // 搜索区域组合控件
    QWidget* searchContainer;
    QHBoxLayout* searchLayout;
    QLabel* lbSearchIcon;
    QLineEdit* editSearch;
    QPushButton* btnAdd;

    QListWidget* listWidget;

    QString currentId;      // 当前选中的好友，空 = 没选中
};

#endif // FRIENDLIST_H