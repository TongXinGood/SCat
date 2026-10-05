#include "../include/ThemeOption.h"
#include "../../Other/include/Theme.h"
#include <QPainter>
#include <QPainterPath>

static const int kMinThumbW = 92;     // 设置页窄的时候三个平分，最窄缩到这么宽
static const int kThumbH = 80;
static const int kLabelH = 30;
static const int kIdealW = 136;

ThemeOption::ThemeOption(Kind kind, const QString& text, QWidget* parent)
    : QAbstractButton(parent), kind(kind), hovered(false)
{
    setText(text);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);

    // 横向可以伸缩，三个选项平分一行；高度固定
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(kMinThumbW + 4);
}

QSize ThemeOption::sizeHint() const
{
    return QSize(kIdealW, kThumbH + 4 + kLabelH);
}

void ThemeOption::enterEvent(QEnterEvent* event)
{
    hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ThemeOption::leaveEvent(QEvent* event)
{
    hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ThemeOption::drawMiniWindow(QPainter& p, const QRectF& r, bool dark) const
{
    // 颜色跟真实界面一致，看缩略图就知道选了以后长什么样。
    // 这里故意不走 Theme：浅色缩略图在深色模式下也得画成浅色的
    QColor bg = dark ? QColor("#15151C") : QColor("#F2F0F5");
    QColor panel = dark ? QColor("#1E1E27") : QColor("#FFFFFF");
    QColor line = dark ? QColor("#34313F") : QColor("#E6E3EC");
    QColor bubbleL = dark ? QColor("#1E1E27") : QColor("#FFFFFF");
    QColor bubbleR = dark ? QColor("#6C5CE7") : QColor("#20202E");

    p.fillRect(r, bg);

    // 左边的好友列表
    QRectF side(r.left(), r.top(), r.width() * 0.34, r.height());
    p.fillRect(side, panel);
    for (int i = 0; i < 3; ++i) {
        qreal y = side.top() + 12 + i * 22;
        p.setBrush(line);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(side.left() + 11, y + 4), 6, 6);
        p.drawRoundedRect(QRectF(side.left() + 21, y, side.width() - 28, 4), 2, 2);
        p.drawRoundedRect(QRectF(side.left() + 21, y + 6, (side.width() - 28) * 0.6, 3), 1.5, 1.5);
    }

    // 右边的聊天气泡：对方的在左、自己的在右
    qreal cx = side.right() + 8;
    qreal cw = r.right() - cx - 8;
    p.setBrush(bubbleL);
    p.drawRoundedRect(QRectF(cx, r.top() + 14, cw * 0.62, 12), 5, 5);
    p.setBrush(bubbleR);
    p.drawRoundedRect(QRectF(cx + cw * 0.38, r.top() + 32, cw * 0.62, 12), 5, 5);
    p.setBrush(bubbleL);
    p.drawRoundedRect(QRectF(cx, r.top() + 50, cw * 0.45, 12), 5, 5);
}

void ThemeOption::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF thumb(2, 2, width() - 4, kThumbH);
    QPainterPath clip;
    clip.addRoundedRect(thumb, 10, 10);

    // 缩略图本身
    p.save();
    p.setClipPath(clip);
    if (kind == System) {
        // 跟随系统：左上半边浅色、右下半边深色，斜着切开
        drawMiniWindow(p, thumb, false);
        QPainterPath half;
        half.moveTo(thumb.topRight());
        half.lineTo(thumb.bottomRight());
        half.lineTo(thumb.bottomLeft());
        half.closeSubpath();
        p.setClipPath(clip.intersected(half));
        drawMiniWindow(p, thumb, true);
    }
    else {
        drawMiniWindow(p, thumb, kind == Dark);
    }
    p.restore();

    // 边框：选中紫色加粗，悬停浅紫，平时淡灰
    QColor accent("#7B6CF0");
    QColor border = isChecked() ? accent
        : hovered ? Theme::bg(QColor("#DCD4F0")) : Theme::bg(QColor("#E6E3EC"));
    p.setPen(QPen(border, isChecked() ? 2.0 : 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);

    // 选中的右上角打个勾
    if (isChecked()) {
        QPointF c(thumb.right() - 12, thumb.top() + 12);
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawEllipse(c, 8, 8);
        p.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tick;
        tick.moveTo(c + QPointF(-3.5, 0.2));
        tick.lineTo(c + QPointF(-1, 2.8));
        tick.lineTo(c + QPointF(3.8, -2.4));
        p.drawPath(tick);
    }

    // 下面的文字
    QFont f = font();
    f.setPixelSize(13);
    f.setWeight(isChecked() ? QFont::DemiBold : QFont::Normal);
    p.setFont(f);
    p.setPen(isChecked() ? Theme::text(QColor("#1A1A1A")) : Theme::text(QColor("#555555")));
    p.drawText(QRectF(0, thumb.bottom() + 4, width(), kLabelH - 4), Qt::AlignHCenter | Qt::AlignVCenter, text());
}