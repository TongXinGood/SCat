#include "../include/NotifyButton.h"
#include <QPainter>
#include <QFontMetrics>

NotifyButton::NotifyButton(QWidget* parent)
    : QPushButton(parent), badgeCount(0)
{
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
}

void NotifyButton::setCount(int count)
{
    if (count < 0)
        count = 0;

    if (badgeCount == count)
        return;

    badgeCount = count;
    update();      // 触发重绘，把角标刷出来
}

void NotifyButton::paintEvent(QPaintEvent* event)
{
    // 先让基类按样式表把圆形底色和铃铛图标画好，我们只在上面补一个角标
    QPushButton::paintEvent(event);

    if (badgeCount <= 0)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QString text = badgeCount > 99 ? QStringLiteral("99+")
        : QString::number(badgeCount);

    QFont f = font();
    f.setPointSize(9);
    f.setBold(true);
    p.setFont(f);

    // 一位数是正圆，两位以上自动拉成胶囊形
    const int h = 18;
    int w = qMax(h, QFontMetrics(f).horizontalAdvance(text) + 10);

    QRect badge(width() - w - 1, 1, w, h);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#FF4D4F"));
    p.drawRoundedRect(badge, h / 2.0, h / 2.0);

    p.setPen(Qt::white);
    p.drawText(badge, Qt::AlignCenter, text);
}