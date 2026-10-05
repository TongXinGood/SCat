#include "../include/Theme.h"
#include <QApplication>
#include <QStyleHints>
#include <QPalette>
#include <QSettings>
#include <QHash>
#include <QRegularExpression>
#include <QDebug>
#include <QImage>

namespace Theme
{

    static const char* kModeKey = "appearance/theme";

    static Mode savedMode = Light;     // 设置里存的
    static bool dark = false;          // 这次启动实际用的

    // ============ 深色对照表 ============
    // 左边是代码里现在写的浅色，右边是深色模式下换成的颜色。
    // 底色表管 background、border 这类；文字表管 color、selection-color。
    // 同一个 #FFFFFF，当底色是白卡片（换成深灰），当字色是深色按钮上的白字（保持白色），
    // 所以要分两张表。表里没有的颜色原样保留，比如未读红点的红色

    static const QHash<QString, QString> kBgMap = {
        // 面板、卡片、窗口
        { "#FFFFFF", "#1E1E27" },
        { "#F2F0F5", "#15151C" },      // 聊天区、设置页的背景

        // 输入框、浅灰底
        { "#FAFAFA", "#262631" },
        { "#FAFAFC", "#262631" },
        { "#F7F7F9", "#262631" },
        { "#F7F6FA", "#24222F" },      // 列表悬停
        { "#F5F5F5", "#2A2A36" },
        { "#F0F0F0", "#2A2A36" },
        { "#F2F2F2", "#2A2A36" },

        // 边框、分割线
        { "#EEEEEE", "#2E2E3A" },
        { "#EDEDED", "#2E2E3A" },
        { "#EAEAEA", "#2E2E3A" },
        { "#E8E8ED", "#2E2E3A" },
        { "#E5E5E5", "#2E2E3A" },
        { "#E4E4E4", "#34343F" },
        { "#E0E0E0", "#34343F" },
        { "#E6E3EC", "#2B2934" },
        { "#E2E0E8", "#33303F" },

        // 浅紫色系：悬停、选中、按下
        { "#F4F1FA", "#2B2740" },
        { "#EFEBFA", "#2F2A48" },
        { "#ECE9F2", "#2E2B3A" },
        { "#E7E1F5", "#383152" },
        { "#E4DCF7", "#383152" },
        { "#DCD4F0", "#433A63" },
        { "#E6E2EE", "#25232F" },      // 时间分隔条
        { "#F0EDF6", "#2A2836" },      // 进度条底
        { "#DCD6E6", "#3A3747" },      // 滚动条
        { "#C9C1D8", "#4A4658" },

        // 主色：自己的气泡、主按钮。深色下换成紫色
        { "#20202E", "#6C5CE7" },
        { "#000000", "#6C5CE7" },
        { "#35354A", "#7B6CF0" },      // 悬停
        { "#333333", "#7B6CF0" },
        { "#16161F", "#5A4BD6" },      // 按下
        { "#1A1A1A", "#5A4BD6" },
        { "#4A4A66", "#8A7DF3" },      // 自己气泡里选中文字的底色
    };

    static const QHash<QString, QString> kTextMap = {
        { "#000000", "#ECEAF2" },
        { "#1A1A1A", "#ECEAF2" },
        { "#333333", "#E2DFEA" },
        { "#3A3A4A", "#DCD8E8" },
        { "#555555", "#B8B5C4" },
        { "#666666", "#B8B5C4" },
        { "#757575", "#8E8A9C" },
        { "#999999", "#8E8A9C" },
        { "#9E9E9E", "#8E8A9C" },
        { "#7E7A8C", "#9C98AA" },
        { "#A3A1AB", "#7D7A8A" },
        { "#AAAAAA", "#6A6778" },
        { "#BBBBBB", "#6A6778" },
        { "#C0C0C0", "#6A6778" },
        { "#20202E", "#FFFFFF" },      // 浅紫底上的深色字，深色下底变深了，字要变白
        { "#FFFFFF", "#FFFFFF" },
    };

    static QString mapHex(const QString& hex, bool isText)
    {
        const QHash<QString, QString>& map = isText ? kTextMap : kBgMap;
        return map.value(hex.toUpper(), hex);
    }

    // 深色模式下 Qt 自带控件（提示框、没写样式的输入框等）用的调色板
    static void applyDarkPalette()
    {
        QPalette pal;
        pal.setColor(QPalette::Window, QColor("#1E1E27"));
        pal.setColor(QPalette::Base, QColor("#1E1E27"));
        pal.setColor(QPalette::AlternateBase, QColor("#262631"));
        pal.setColor(QPalette::Button, QColor("#262631"));
        pal.setColor(QPalette::WindowText, QColor("#ECEAF2"));
        pal.setColor(QPalette::Text, QColor("#ECEAF2"));
        pal.setColor(QPalette::ButtonText, QColor("#ECEAF2"));
        pal.setColor(QPalette::PlaceholderText, QColor("#6A6778"));
        pal.setColor(QPalette::Highlight, QColor("#433A63"));
        pal.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
        pal.setColor(QPalette::ToolTipBase, QColor("#262631"));
        pal.setColor(QPalette::ToolTipText, QColor("#ECEAF2"));
        qApp->setPalette(pal);
    }

    void init()
    {
        QSettings settings;
        int value = settings.value(kModeKey, Light).toInt();
        savedMode = (value == Dark || value == System) ? Mode(value) : Light;

        dark = (savedMode == Dark);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        // 跟随系统：问一下 Windows 现在是深色还是浅色。
        // 必须在下面把配色方案写死之前问，写死之后拿到的就是写死的值了
        if (savedMode == System)
            dark = (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);

        // 告诉 Qt 自带的控件用哪套颜色画，从此不再跟着系统变。
        // 原来 main.cpp 里写死浅色，是为了修"系统是深色时字看不清"的问题，这里接着管
        QGuiApplication::styleHints()->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#endif

        // 调色板要在配色方案之后设，不然会被 Qt 按配色方案重新算一遍盖掉
        if (dark)
            applyDarkPalette();

        qDebug() << "theme:" << (savedMode == Light ? "light" : savedMode == Dark ? "dark" : "system")
            << (dark ? "-> dark" : "-> light");
    }

    Mode mode()
    {
        return savedMode;
    }

    void setMode(Mode mode)
    {
        savedMode = mode;

        QSettings settings;
        settings.setValue(kModeKey, int(mode));
    }

    bool isDark()
    {
        return dark;
    }

    QString css(const QString& qss)
    {
        // 浅色原样返回，保证浅色界面跟以前一个像素都不差
        if (!dark)
            return qss;

        // 一条一条声明地处理："属性: 值"。冒号后面、分号或花括号前面的是值。
        // 选择器里的 "QPushButton:hover" 也会被当成一条，但它的值里没有颜色，不受影响
        static const QRegularExpression declRe("([A-Za-z-]+)(\\s*:\\s*)([^;{}]*)");
        static const QRegularExpression hexRe("#[0-9A-Fa-f]{6}\\b");
        static const QRegularExpression shortRe("#([0-9A-Fa-f])([0-9A-Fa-f])([0-9A-Fa-f])\\b");
        static const QRegularExpression shadeRe("rgba\\(\\s*0\\s*,\\s*0\\s*,\\s*0\\s*,");

        QString out;
        int last = 0;
        QRegularExpressionMatchIterator it = declRe.globalMatch(qss);

        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            QString prop = m.captured(1).toLower();
            QString value = m.captured(3);

            // #333 这种三位简写先展开成 #333333，再统一按六位查表
            value.replace(shortRe, "#\\1\\1\\2\\2\\3\\3");

            bool isText = (prop == "color" || prop == "selection-color");

            QString mapped;
            int pos = 0;
            QRegularExpressionMatchIterator hexIt = hexRe.globalMatch(value);
            while (hexIt.hasNext()) {
                QRegularExpressionMatch h = hexIt.next();
                mapped += value.mid(pos, h.capturedStart() - pos);
                mapped += mapHex(h.captured(), isText);
                pos = h.capturedEnd();
            }
            mapped += value.mid(pos);

            // 浅色下"压一层半透明黑"表示悬停，深色底上看不出来，换成半透明白
            if (!isText)
                mapped.replace(shadeRe, "rgba(255, 255, 255,");

            out += qss.mid(last, m.capturedStart(3) - last);
            out += mapped;
            last = m.capturedEnd(3);
        }

        out += qss.mid(last);
        return out;
    }

    static QColor mapColor(const QColor& light, bool isText)
    {
        if (!dark)
            return light;

        QColor c(mapHex(light.name(QColor::HexRgb), isText));
        c.setAlpha(light.alpha());     // 透明度保持原样，只换颜色
        return c;
    }

    QColor bg(const QColor& light)
    {
        return mapColor(light, false);
    }

    QColor text(const QColor& light)
    {
        return mapColor(light, true);
    }
    QPixmap pixmap(const QString& path, const QColor& darkColor)
    {
        if (!dark)
            return QPixmap(path);

        QImage img = QImage(path).convertToFormat(QImage::Format_ARGB32);

        // 先找出图标里最深的那个灰度，染色时把它当成"完全不透明"。
        // 项目里的图标有的是中灰（130）画的，直接按黑色算会染得很淡
        int darkest = 255;
        for (int y = 0; y < img.height(); ++y) {
            const QRgb* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                if (qAlpha(line[x]) > 128)
                    darkest = qMin(darkest, qGray(line[x]));
            }
        }
        int range = qMax(1, 255 - darkest);

        // 越深的地方越不透明，白色（包括自带的白底）变透明
        for (int y = 0; y < img.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                int depth = qMin(255, (255 - qGray(line[x])) * 255 / range);
                int alpha = qAlpha(line[x]) * depth / 255;
                line[x] = qRgba(darkColor.red(), darkColor.green(), darkColor.blue(), alpha);
            }
        }

        return QPixmap::fromImage(img);
    }

    QIcon icon(const QString& path, const QColor& darkColor)
    {
        if (!dark)
            return QIcon(path);

        return QIcon(pixmap(path, darkColor));
    }
}