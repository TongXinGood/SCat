#ifndef LOGIN_H
#define LOGIN_H

#include <QObject>
#include <QJsonObject>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"

class NetWorkManager;

class Login : public QObject
{
    Q_OBJECT

public:
    explicit Login(NetWorkManager* net, QObject* parent = nullptr);

public slots:
    // 接 LoginWindow::sendLoginClicked
    void doLogin(const QString& account, const QString& pwd);

signals:
    // 转发给上层：登录成功 / 失败
    void loginSuccess(const QJsonObject& info);
    void loginFailed(const QString& reason);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleLoginResp(const QJsonObject& obj);

private:
    NetWorkManager* net;
    QString pendingAccount;    // 正在登录的账号，收到响应时用
};

#endif