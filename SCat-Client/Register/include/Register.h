#ifndef REGISTER_H
#define REGISTER_H

#include <QObject>
#include <QJsonObject>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"
#include <QRegularExpression>
#include <QDebug>
class NetWorkManager;

class Register : public QObject
{
    Q_OBJECT

public:
    explicit Register(NetWorkManager* net, QObject* parent = nullptr);

public slots:
    // 接 RegisterWindow::sendRegisterClicked
    void doRegister(const QString& account, const QString& pwd);

signals:
    void registerSuccess(const QString& account);
    void registerFailed(const QString& reason);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);

private:
    void handleRegisterResp(const QJsonObject& obj);

private:
    NetWorkManager* net;
    QString pendingAccount;
};

#endif // REGISTER_H