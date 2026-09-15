#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Login/include/Login.h"
#include "../../Login/include/LoginWindow.h"
#include "../../Register/include/Register.h"
#include "../../Register/include/RegisterWindow.h"
#include "../../Scat/include/ScatWindow.h"
#include "../../Friend/include/FriendManager.h"
#include "../../Chat/include/ChatNetWork.h"
#include "../../Chat/include/ChatStorage.h"
#include "../../Chat/include/ChatMessage.h"
#include "../../Friend/include/AddFriendWindow.h"
#include "../../Friend/include/RequestWindow.h"
#include "../../Setting/include/SettingsManager.h"
#include "../include/AvatarUtils.h" 
#include "../include/Notification.h"

class NetWorkManager;
class Login;
class Register;
class LoginWindow;
class RegisterWindow;
class ScatWindow;
class FriendManager;
class ChatStorage;
class ChatNetWork;
class AddFriendWindow;
class RequestWindow;
class SettingsManager;
class Notification;


class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    void start();

private slots:
    // 窗口切换
    void showLoginWindow();
    void showRegisterWindow();
    void showAddFriendWindow();
    void onNotifyClicked();

    // 业务结果
    void onLoginSuccess(const QJsonObject& info);
    void onLoginFailed(const QString& reason);
    void onRegisterSuccess(const QString& account);
    void onRegisterFailed(const QString& reason);

    // 好友申请
    void onPendingListReady(const QJsonArray& requests);
    void onNewRequestArrived(const QString& username, const QString& nickname,const QString& avatar);
    void onHandleRequest(const QString& username, int action);
    void onRequestHandled(bool ok, const QString& username, int action);
    void onFriendListChanged();

    // 好友列表
    void onFriendListReady(const QJsonArray& friends);
    void onFriendListFailed(const QString& reason);
    void onFriendStatusChanged(const QString& username, bool online);

    // 聊天
    void onSendTextMessage(const QString& to, const QString& content);
    void onRequestHistory(const QString& friendId);
    void onMessageSent(const ChatMessage& msg);
    void onMessageReceived(const ChatMessage& msg);
    void onChatSendFailed(const QString& reason);

    // 网络状态
    void onNetError(const QString& msg);

    // 设置
    void onSaveNickname(const QString& nickname);
    void onNicknameSaved(bool ok, const QString& nickname, const QString& reason);
    void onChangeStorage(const QString& dir);
    void onChangeAvatar(const QString& filePath);
    void onAvatarUploaded(bool ok, const QString& avatar, const QString& reason);
    void onAvatarDownloaded(const QString& avatar);

    void onNotificationClicked(const QString& peer);
    void onTrayActivated();
private:
    void updatePendingUi();

    NetWorkManager* net;
    Login* loginLogic;
    Register* regLogic;

    LoginWindow* loginWin;
    RegisterWindow* regWin;
    ScatWindow* scatWin;
    FriendManager* friendMgr;
    ChatNetWork* chatNet;
    ChatStorage* storage;
    AddFriendWindow* addFriendWin;
    RequestWindow* requestWin;
    int pendingCount;     
    QJsonArray pendingRequests;
    SettingsManager* settingsMgr;
    Notification* notify;
};

#endif // !APPCONTROLLER_H
