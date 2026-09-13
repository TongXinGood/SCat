#include "../include/AppController.h"
#include <QMessageBox>
#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , net(nullptr)
    , loginLogic(nullptr)
    , regLogic(nullptr)
    , loginWin(nullptr)
    , regWin(nullptr)
    , scatWin(nullptr)
{
    // 网络层
    net = new NetWorkManager(this);

    // 业务层（注入网络）
    loginLogic = new Login(net, this);
    regLogic = new Register(net, this);

    // 业务结果
    connect(loginLogic, &Login::loginSuccess, this, &AppController::onLoginSuccess);
    connect(loginLogic, &Login::loginFailed, this, &AppController::onLoginFailed);
    connect(regLogic, &Register::registerSuccess, this, &AppController::onRegisterSuccess);
    connect(regLogic, &Register::registerFailed, this, &AppController::onRegisterFailed);

    connect(net, &NetWorkManager::errorOccurred, this, &AppController::onNetError);
}

AppController::~AppController()
{
    delete loginWin;
    delete regWin;
    delete scatWin;
}

void AppController::start()
{
    net->connectToServer("127.0.0.1", 8888);
    showLoginWindow();
}

// ---------- 窗口切换 ----------

void AppController::showLoginWindow()
{
    if (regWin)
        regWin->hide();

    if (!loginWin) {
        loginWin = new LoginWindow;

        connect(loginWin, &LoginWindow::sendLoginClicked,
            loginLogic, &Login::doLogin);
        connect(loginWin, &LoginWindow::sendRegisterClicked,
            this, &AppController::showRegisterWindow);
    }

    loginWin->show();
}

void AppController::showRegisterWindow()
{
    if (loginWin)
        loginWin->hide();

    if (!regWin) {
        regWin = new RegisterWindow;

        connect(regWin, &RegisterWindow::sendRegisterClicked,
            regLogic, &Register::doRegister);
        connect(regWin, &RegisterWindow::sendReturnClicked,
            this, &AppController::showLoginWindow);
    }

    regWin->show();
}

// ---------- 业务结果 ----------

void AppController::onLoginSuccess(const QJsonObject& info)
{
    QString nickname = info["nickname"].toString();
    QString avatar = info["avatar"].toString();

    scatWin = new ScatWindow;
    scatWin->setUserInfo(nickname, avatar);
    scatWin->show();

    // 登录窗用不着了
    if (loginWin) {
        loginWin->close();
        loginWin->deleteLater();
        loginWin = nullptr;
    }
    if (regWin) {
        regWin->close();
        regWin->deleteLater();
        regWin = nullptr;
    }
}

void AppController::onLoginFailed(const QString& reason)
{
    QMessageBox::warning(loginWin, "登录失败", reason);
}

void AppController::onRegisterSuccess(const QString& account)
{
    QMessageBox::information(regWin, "注册成功",
        "账号 " + account + " 注册成功，请登录");
    showLoginWindow();
}

void AppController::onRegisterFailed(const QString& reason)
{
    QMessageBox::warning(regWin, "注册失败", reason);
}

// ---------- 网络 ----------

void AppController::onNetError(const QString& msg)
{
    qDebug() << "network error:" << msg;
}