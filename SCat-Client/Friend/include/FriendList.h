#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class FriendList : public QWidget
{
    Q_OBJECT
public:
    explicit FriendList(QWidget* parent = nullptr);
    ~FriendList();

    void addFriendItem(const QString& id, const QPixmap& avatar,const QString& name, const QString& lastMsg);
    void updateAvatar(const QString& id, const QPixmap& avatar);
    void clearFriends();
    void clearSelection();
    void updateLastMessage(const QString& id, const QString& msg);
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
    QString currentId;

    // 搜索区域组合控件
    QWidget* searchContainer;
    QHBoxLayout* searchLayout;
    QLabel* lbSearchIcon;
    QLineEdit* editSearch;
    QPushButton* btnAdd;

    QListWidget* listWidget;
};

#endif // FRIENDLIST_H