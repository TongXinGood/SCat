#include "../include/Notification.h"
#include "../include/MessageToast.h"
#include <QApplication>
#include <QMenu>
#include <QIcon>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>

static const int kMaxToasts = 3;     // 屏幕上最多同时几个，再多把最旧的挤掉
static const int kToastShadow = 14;  // 弹窗四周透明阴影边的宽度，跟 MessageToast 里的一致

Notification::Notification(QObject* parent)
    : QObject(parent), tray(nullptr)
{
    // 有些环境（某些 Linux 桌面、精简版系统）没有托盘，得先判断。
    // 没有托盘只是少了托盘图标，消息弹窗是自己画的，照样能弹
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "system tray not available, tray icon disabled";
        return;
    }

    tray = new QSystemTrayIcon(this);
    // 图标必须设，没图标托盘项不显示
    tray->setIcon(QIcon(":/Resource/icon/logo.png"));
    tray->setToolTip("SCat");

    QMenu* menu = new QMenu();
    menu->addAction("打开 SCat", this, &Notification::trayActivated);
    menu->addSeparator();
    menu->addAction("退出", qApp, &QApplication::quit);
    tray->setContextMenu(menu);

    connect(tray, &QSystemTrayIcon::activated,
        this, &Notification::onTrayActivated);

    tray->show();
}

void Notification::showMessage(const QString& peer, const QString& title,
    const QString& content, const QPixmap& avatar)
{
    // 弹窗是自己画的，不依赖托盘。没有托盘的环境照样能弹

    // 这个人的弹窗还在：原地更新，条数加一，重新计时
    for (MessageToast* toast : toasts) {
        if (toast->peer() == peer) {
            toast->setMessage(peer, title, avatar, content, toast->count() + 1);
            toast->restartTimer();
            return;
        }
    }

    // 满了就把最旧的那个关掉，给新的腾位置
    if (toasts.size() >= kMaxToasts)
        toasts.first()->dismiss();

    MessageToast* toast = new MessageToast();
    toast->setMessage(peer, title, avatar, content, 1);

    connect(toast, &MessageToast::clicked, this, &Notification::notificationClicked);
    connect(toast, &MessageToast::closed, this, &Notification::onToastClosed);

    toasts.append(toast);
    layoutToasts(toast);
}

void Notification::onToastClosed(MessageToast* toast)
{
    toasts.removeOne(toast);

    // 中间空出来一个位置，上面的往下挪
    layoutToasts();
}

void Notification::layoutToasts(MessageToast* fresh)
{
    // 贴着屏幕可用区域的右下角摆。availableGeometry 已经扣掉了任务栏，
    // 任务栏在左边、上边也一样能用
    QRect area = QGuiApplication::primaryScreen()->availableGeometry();
    int bottom = area.bottom() + 1;

    // 新的在最下面，旧的往上叠。每个弹窗四周都带了一圈 14px 的透明阴影边，
    // 叠的时候让相邻两个的阴影边重叠，卡片之间的空隙就正好是 14px
    for (int i = toasts.size() - 1; i >= 0; --i) {
        MessageToast* toast = toasts.at(i);
        QPoint pos(area.right() + 1 - toast->width(), bottom - toast->height());

        if (toast == fresh)
            toast->popup(pos);
        else
            toast->moveTo(pos);

        bottom = pos.y() + kToastShadow;
    }
}

void Notification::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // 双击托盘图标把窗口叫回来
    if (reason == QSystemTrayIcon::DoubleClick)
        emit trayActivated();
}