#ifndef MOTION_H
#define MOTION_H

#include <QEasingCurve>
#include <QPoint>
#include <QColor>

class QWidget;

// Material 3 风格的动画。曲线和时长照搬 Material 规范，
// 全项目的动画都从这里取，节奏才统一。
// Windows 设置里关掉了"动画效果"的话，这里的动画全部直接跳到结尾
namespace Motion
{
    // 时长（毫秒）
    static const int kShort = 150;      // 悬停、按下这类小变化
    static const int kMedium = 300;     // 水波纹淡出
    static const int kLong = 450;       // 面板展开、新消息出现、水波纹扩散

    // 缓动曲线
    QEasingCurve emphasizedDecelerate();    // 东西出现：一开始很快，后面慢慢停稳
    QEasingCurve emphasizedAccelerate();    // 东西消失：慢慢起步，越走越快
    QEasingCurve standard();                // 普通的移动、变色

    bool enabled();     // 系统允许动画吗

    // 水波纹的颜色：深色按钮（登录、发送）上用白色，浅色底上用深色
    enum RippleTone
    {
        OnSurface,      // 白卡片、列表、浅色按钮
        OnPrimary,      // 黑色 / 紫色的主按钮
    };

    // 给任意控件加上点击水波纹。不改控件本身，样式表照常生效。
    // radius 跟控件样式表里的 border-radius 一致，波纹按这个圆角裁剪
    void addRipple(QWidget* target, int radius, RippleTone tone = OnSurface);

    // 新消息出现：从下面上浮一小段，同时淡入。
    // widget 必须有布局，靠调布局的上下边距来挪，不影响列表算高度
    void slideUpIn(QWidget* widget);

    // 弹出面板展开：从 origin（面板坐标，一般是按钮所在的那个角）放大并淡入。
    // content 是面板里真正画东西的那一层（圆角卡片），动画期间先藏起来，用截图代替
    void popupOpen(QWidget* popup, QWidget* content, const QPoint& origin);
}

#endif // MOTION_H