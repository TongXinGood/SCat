#include "../include/Login.h"
#include <QDebug>

Login::Login(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &Login::onPacketReceived);
}

void Login::doLogin(const QString& account, const QString& pwd)
{
    if (!net->isConnected()) {
        emit loginFailed("未连接到服务器");
        return;
    }

    pendingAccount = account;

    QJsonObject obj;
    obj["username"] = account;
    obj["password"] = pwd;

    net->sendPacket(MSG_LOGIN_REQ, obj);
    qDebug() << "login request sent:" << account;
}

void Login::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    // 只处理跟自己有关的包，其它的忽略
    if (type != MSG_LOGIN_RESP)
        return;

    handleLoginResp(obj);
}

void Login::handleLoginResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code == ERR_OK) {
        qDebug() << "login success:" << pendingAccount;
        emit loginSuccess(obj);          // 直接把整个响应传出去
    }
    else {
        qDebug() << "login failed, code:" << code << obj["msg"].toString();

        switch (code) {
        case ERR_USER_NOT_FOUND:  emit loginFailed("用户不存在"); break;
        case ERR_WRONG_PASSWORD:  emit loginFailed("密码错误"); break;
        default: emit loginFailed(QString("登录失败 (code: %1)").arg(code)); break;
        }
    }

    pendingAccount.clear();
}