#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QJsonObject>
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

class NetWorkManager;
class Login;
class Register;
class LoginWindow;
class RegisterWindow;
class ScatWindow;
class FriendManager;
class ChatStorage;
class ChatNetWork;

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

    // 业务结果
    void onLoginSuccess(const QJsonObject& info);
    void onLoginFailed(const QString& reason);
    void onRegisterSuccess(const QString& account);
    void onRegisterFailed(const QString& reason);

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

private:
    NetWorkManager* net;
    Login* loginLogic;
    Register* regLogic;

    LoginWindow* loginWin;
    RegisterWindow* regWin;
    ScatWindow* scatWin;
    FriendManager* friendMgr;
    ChatNetWork* chatNet;
    ChatStorage* storage;
};

#endif // !APPCONTROLLER_H
