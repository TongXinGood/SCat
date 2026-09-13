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

class NetWorkManager;
class Login;
class Register;
class LoginWindow;
class RegisterWindow;
class ScatWindow;

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

    // 网络状态
    void onNetError(const QString& msg);

private:
    NetWorkManager* net;
    Login* loginLogic;
    Register* regLogic;

    LoginWindow* loginWin;
    RegisterWindow* regWin;
    ScatWindow* scatWin;
};

#endif // !APPCONTROLLER_H
