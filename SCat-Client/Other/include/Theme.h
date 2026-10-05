#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QPixmap>
#include <QIcon>

// 浅色 / 深色主题。全项目的颜色还是照常写浅色的，
// 设样式表的时候过一道 Theme::css()，深色模式下自动换成对应的深色。
// 想调深色的某个颜色，只改 Theme.cpp 里那两张对照表
namespace Theme
{
    enum Mode
    {
        Light = 0,      // 浅色（默认）
        Dark = 1,       // 深色
        System = 2,     // 跟随 Windows 的设置
    };

    // 程序启动时调一次，必须在创建任何窗口之前。
    // 读出用户选的主题；选的是"跟随系统"的话，顺便看一眼 Windows 现在是深色还是浅色
    void init();

    Mode mode();                // 用户在设置页选的是哪个
    void setMode(Mode mode);    // 存起来，重启后生效
    bool isDark();              // 这次启动实际用的是不是深色

    // 把一段按浅色写的样式表换成当前主题的。浅色模式原样返回；
    // 深色模式按属性查表：color 这类文字颜色查文字表，background、border 查底色表
    QString css(const QString& qss);

    // 代码里直接画的颜色（QPainter、QColor）用这两个换，规则跟 css 一样
    QColor bg(const QColor& light);
    QColor text(const QColor& light);

    // 单色图标（灰色、黑色的线条图标）。浅色模式原样读出来；
    // 深色模式下把图标染成 darkColor，不然深色底上看不清。
    // 有的图标自带白底，染色时白底会变透明
    QPixmap pixmap(const QString& path, const QColor& darkColor = QColor("#B8B5C4"));
    QIcon icon(const QString& path, const QColor& darkColor = QColor("#B8B5C4"));
}

#endif // THEME_H