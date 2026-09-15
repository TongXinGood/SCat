#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>
#include <QSystemTrayIcon>

// 托盘图标 + 桌面通知（Windows 右下角弹的那种）。
// 底层用 QSystemTrayIcon，Qt 会自动调系统的通知机制，
// 不用碰任何 Windows API，也顺带跨平台了
class Notification : public QObject
{
    Q_OBJECT

public:
    explicit Notification(QObject* parent = nullptr);

    bool isAvailable() const { return tray != nullptr; }

    // 弹一条桌面通知。peer 是发消息的人，点通知时要跳到他的会话
    void showMessage(const QString& peer, const QString& title, const QString& content);

signals:
    void notificationClicked(const QString& peer);   // 用户点了通知气泡
    void trayActivated();                            // 用户双击了托盘图标

private slots:
    void onMessageClicked();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon* tray;
    QString lastPeer;      // 最近一条通知是谁发的，点击时用它跳转
};

#endif // NOTIFICATION_H