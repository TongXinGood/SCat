#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>
#include <QIcon>
#include <QPixmap>
#include <QList>
#include <QSystemTrayIcon>

class MessageToast;

// 托盘图标 + 新消息弹窗。
// 弹窗以前用系统托盘气泡，Windows 会把头像缩成二十来像素的小图标，
// 表情也是系统样式，现在换成自己画的 MessageToast，托盘图标和右键菜单不变
class Notification : public QObject
{
    Q_OBJECT

public:
    explicit Notification(QObject* parent = nullptr);

    bool isAvailable() const { return tray != nullptr; }

    // 右下角弹一条新消息。peer 是发消息的人，点弹窗时要跳到他的会话。
    // 同一个人的弹窗还在的话不再新弹，在原来那个上面更新内容、条数加一
    void showMessage(const QString& peer, const QString& title,
        const QString& content, const QPixmap& avatar);

signals:
    void notificationClicked(const QString& peer);   // 用户点了消息弹窗
    void trayActivated();                            // 用户双击了托盘图标

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onToastClosed(MessageToast* toast);

private:
    void layoutToasts(MessageToast* fresh = nullptr);

private:
    QSystemTrayIcon* tray;
    QList<MessageToast*> toasts;   // 屏幕上的弹窗，旧的在前、新的在后
};

#endif // NOTIFICATION_H