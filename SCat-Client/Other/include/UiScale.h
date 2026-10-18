#ifndef UISCALE_H
#define UISCALE_H

#include <QList>

// 界面缩放：在 Windows 的缩放基础上再整体缩小一档。
// 比如 Windows 设成 150%、这里选 75%，SCat 实际按 112.5% 画。
// 原理是启动前设 QT_SCALE_FACTOR，所有窗口、字体、图标、圆角一起等比缩放，
// 代码里写的尺寸都不用动。改了要重启才生效，跟换主题一样
namespace UiScale
{
    QList<int> options();       // 可选的档位（百分比）：100、90、80、75

    // 程序一启动就调，必须在创建 QApplication 之前，
    // 不然 Qt 已经按原来的比例算好了，再设没用
    void apply();

    int percent();              // 设置里存的档位
    void setPercent(int percent);   // 存起来，重启后生效
}

#endif // UISCALE_H