#ifndef ADDFRIENDWINDOW_H
#define ADDFRIENDWINDOW_H

#include "../../Protocol.h"
#include "../../NoFrame.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 搜到的这个人跟我是什么关系，决定"添加"按钮是可点还是灰的。
// 注意没有"已申请"这一项 —— 只要对方还没同意，就允许重新申请，
// "已申请"只是点过按钮之后的本地反馈，不从服务端查


class AddFriendWindow : public NoFrame
{
    Q_OBJECT

public:
    explicit AddFriendWindow(QWidget* parent = nullptr);
    ~AddFriendWindow();

    // --- 给逻辑层调用的接口 ---
    void showSearching();                       // 请求已发出，等服务端回
    void showNotFound();                        // 服务端说查无此人
    void showResult(const QString& username, const QString& nickname,
        const QString& avatar, int relation);
    void onAddSent(bool ok, const QString& reason);   // 申请发送的结果
    void clearResult();                         // 清空，回到初始状态

signals:
    void sendSearchUser(const QString& username);   // 用户按了回车
    void sendAddFriend(const QString& username);    // 用户点了添加

private slots:
    void onSearchReturn();
    void onAddClicked();

private:
    void initUI();
    void initConnect();

    // 显示一行提示文字，同时把结果行藏起来
    void setHint(const QString& text);

private:
    QWidget* content;
    QVBoxLayout* mainLayout;

    // 搜索栏
    QWidget* searchContainer;
    QLabel* lbSearchIcon;
    QLineEdit* editSearch;

    // 提示文字：空态 / 搜索中 / 未找到
    QLabel* lbHint;

    // 搜索结果行
    QWidget* resultWidget;
    QLabel* lbAvatar;
    QLabel* lbNickname;
    QLabel* lbUsername;
    QPushButton* btnAdd;

    QString resultUser;     // 当前结果这个人的 username
};

#endif // ADDFRIENDWINDOW_H