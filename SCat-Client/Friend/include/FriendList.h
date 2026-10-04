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

    // lastTime 是最后一条消息的时间（毫秒），没聊过天传 0，排在最下面
    void addFriendItem(const QString& id, const QPixmap& avatar,
        const QString& name, const QString& lastMsg, qint64 lastTime = 0);
    void clearFriends();
    void clearSelection();

    // 来了新消息（收到的、自己发的都算）：更新副标题，这个人挪到最上面
    void updateLastMessage(const QString& id, const QString& msg, qint64 time);

    void sortByTime();          // 按最后一条消息的时间排，新的在上面
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