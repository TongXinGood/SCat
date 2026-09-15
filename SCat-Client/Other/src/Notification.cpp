#include "../include/Notification.h"
#include <QApplication>
#include <QMenu>
#include <QIcon>
#include <QDebug>

Notification::Notification(QObject* parent)
    : QObject(parent), tray(nullptr)
{
    // 有些环境（某些 Linux 桌面、精简版系统）没有托盘，得先判断
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "system tray not available, notifications disabled";
        return;
    }

    tray = new QSystemTrayIcon(this);
    // 图标必须设，没图标托盘项不显示，通知也弹不出来
    tray->setIcon(QIcon(":/Resource/icon/logo.png"));
    tray->setToolTip("SCat");

    QMenu* menu = new QMenu();
    menu->addAction("打开 SCat", this, &Notification::trayActivated);
    menu->addSeparator();
    menu->addAction("退出", qApp, &QApplication::quit);
    tray->setContextMenu(menu);

    connect(tray, &QSystemTrayIcon::messageClicked,
        this, &Notification::onMessageClicked);
    connect(tray, &QSystemTrayIcon::activated,
        this, &Notification::onTrayActivated);

    tray->show();      // 必须 show，否则 showMessage 无效
}

void Notification::showMessage(const QString& peer, const QString& title,
    const QString& content, const QIcon& icon)
{
    if (!tray)
        return;

    lastPeer = peer;

    // 用 QIcon 这个重载，就不会是系统那个蓝色感叹号了。
    // 传空就退回应用 logo
    QIcon shown = icon.isNull() ? QIcon(":/Resource/icon/logo.png") : icon;

    // 最后那个 5000 是显示时长（毫秒），不过 Windows 10/11 上
    // 实际时长由系统说了算，传什么它不一定听
    tray->showMessage(title, content, shown, 5000);
}

void Notification::onMessageClicked()
{
    if (!lastPeer.isEmpty())
        emit notificationClicked(lastPeer);
}

void Notification::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // 双击托盘图标把窗口叫回来
    if (reason == QSystemTrayIcon::DoubleClick)
        emit trayActivated();
}