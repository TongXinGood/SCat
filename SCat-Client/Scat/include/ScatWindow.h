#ifndef SCATWINDOW_H
#define SCATWINDOW_H

#include "../../NoFrame.h"
#include "../../Friend/include/FriendList.h"
#include "../../Chat/include/ChatWindow.h"
#include "../../Chat/include/ChatMessage.h"
#include "../../Other/include/NotifyButton.h"
#include "../../Setting/include/SettingsPage.h"
#include <QStackedWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHash>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

class ScatWindow : public NoFrame
{
    Q_OBJECT
public:
    explicit ScatWindow(QWidget* parent = nullptr);
    ~ScatWindow();
    void setUserInfo(const QString& nickname, const QString& avatar);
    void setFriendList(const QJsonArray& friends);
    void updateFriendStatus(const QString& username, bool online);
    void loadHistory(const QList<ChatMessage>& list);
    void addChatMessage(const ChatMessage& msg);
    void setRequestCount(int count);    
    void setSettingsInfo(const QString& username, const QString& nickname,const QString& avatar);
    void setStoragePath(const QString& path);
    void onNicknameSaved(bool ok, const QString& reason);
    void onAvatarUploaded(bool ok, const QString& reason);
    void updateMyNickname(const QString& nickname);
    void updateMyAvatar(const QString& avatar);
    void refreshAvatar(const QString& avatar);
    void selectFriend(const QString& username);              // 点通知时切到这个会话
    QString nicknameOf(const QString& username) const;       // 弹通知时拿昵称当标题
    QPixmap avatarOf(const QString& username, int size) const;
signals:
    // 抛出给上层或控制器的信号
    void sendSettingsClicked();
    void sendAddFriendClicked();

    void requestHistory(const QString& friendId);

    // 转发 ChatWindow 的信号，附带当前聊天的 friendId
    void sendTextMessage(const QString& friendId, const QString& msg);
    void sendImage(const QString& friendId, const QImage& image);
    void sendFileClicked(const QString& friendId);
    void sendMoodClicked(const QString& friendId);

    void sendNotifyClicked();

    // 设置页转发上来的
    void sendChangeAvatar(const QString& filePath);
    void sendSaveNickname(const QString& nickname);
    void sendChangeStorage(const QString& dir);
private slots:
    void onFriendSelected(const QString& friendId, const QString& friendName);
    void onSettingsClicked();
    void onFriendUnselected();
    // 捕获 ChatWindow 内部发出的消息，带上身份标识转发出去
    void onChatTextMsgSent(const QString& msg);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initUI();
    void initProfileSection();
    void initConnect();
    void updateNotifyPos();
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

    NotifyButton* btnNotify;

    // 右侧区域 (多视图堆叠)
    QStackedWidget* rightStackedWidget;

    // 页面 0: 默认背景
    QWidget* defaultPage;
    QLabel* lbDefaultBg;

    // 页面 1: 聊天窗口
    ChatWindow* chatWindow;

    SettingsPage* settingsPage;

    // 记录当前正在聊天的目标对象
    QString currentFriendId;
    // username -> 该好友的完整信息（昵称、头像、在线状态）。
    // 点好友时直接从这里取，不用再问服务端
    QHash<QString, QJsonObject> friendInfos;

    QHash<QString, int> unreadCounts;
};

#endif // SCATWINDOW_H