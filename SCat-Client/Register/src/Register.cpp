#include "../include/Register.h"

Register::Register(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &Register::onPacketReceived);
}

void Register::doRegister(const QString& account, const QString& pwd)
{
    // 本地校验
    QRegularExpression re("^[a-zA-Z0-9]{4,15}$");
    if (!re.match(account).hasMatch()) {
        emit registerFailed("账号只能包含字母和数字，长度 4-15 位");
        return;
    }

    if (pwd.length() < 8 || pwd.length() > 16) {
        emit registerFailed("密码长度应为 8-16 位");
        return;
    }

    if (!net->isConnected()) {
        emit registerFailed("未连接到服务器");
        return;
    }

    pendingAccount = account;

    QJsonObject obj;
    obj["username"] = account;
    obj["password"] = pwd;
    obj["nickname"] = account;      // 昵称默认用账号名

    net->sendPacket(MSG_REG_REQ, obj);
    qDebug() << "register request sent:" << account;
}

void Register::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    if (type != MSG_REG_RESP)
        return;

    handleRegisterResp(obj);
}

void Register::handleRegisterResp(const QJsonObject& obj)
{
    int code = obj["code"].toInt();

    if (code == ERR_OK) {
        qDebug() << "register success:" << pendingAccount;
        emit registerSuccess(pendingAccount);
    }
    else {
        qDebug() << "register failed, code:" << code;

        switch (code) {
        case ERR_USER_EXIST: emit registerFailed("该账号已被注册"); break;
        case ERR_DB_ERROR:   emit registerFailed("服务器错误，请稍后再试"); break;
        default:             emit registerFailed(QString("注册失败 (code: %1)").arg(code)); break;
        }
    }

    pendingAccount.clear();
}