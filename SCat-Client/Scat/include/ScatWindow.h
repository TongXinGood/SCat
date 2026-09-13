#ifndef SCATWINDOW_H
#define SCATWINDOW_H

#include "../../NoFrame.h"
#include "../../Friend/include/FriendList.h"
#include "../../Chat/include/ChatWindow.h"
#include <QStackedWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class ScatWindow : public NoFrame
{
    Q_OBJECT
public:
    explicit ScatWindow(QWidget* parent = nullptr);
    ~ScatWindow();
    void setUserInfo(const QString& nickname, const QString& avatar);
signals:
    // 抛出给上层或控制器的信号
    void sendSettingsClicked();
    void sendAddFriendClicked();

    // 转发 ChatWindow 的信号，附带当前聊天的 friendId
    void sendTextMessage(const QString& friendId, const QString& msg);
    void sendFileClicked(const QString& friendId);
    void sendMoodClicked(const QString& friendId);

private slots:
    void onFriendSelected(const QString& friendId, const QString& friendName);
    void onSettingsClicked();

    // 捕获 ChatWindow 内部发出的消息，带上身份标识转发出去
    void onChatTextMsgSent(const QString& msg);

private:
    void initUI();
    void initProfileSection();
    void initConnect();

private:
    // 整个窗口的核心容器，最后会被塞进 NoFrame
    QWidget* centralWidget;
    QHBoxLayout* mainLayout;

    // 左侧区域 (个人信息 + 好友列表)
    QWidget* leftContainer;
    QVBoxLayout* leftLayout;

    QWidget* profileWidget;
    QLabel* lbMyAvatar;
    QLabel* lbMyName;
    QPushButton* btnSettings;

    FriendList* friendList;

    // 右侧区域 (多视图堆叠)
    QStackedWidget* rightStackedWidget;

    // 页面 0: 默认背景
    QWidget* defaultPage;
    QLabel* lbDefaultBg;

    // 页面 1: 聊天窗口
    ChatWindow* chatWindow;

    // 记录当前正在聊天的目标对象
    QString currentFriendId;
};

#endif // SCATWINDOW_H