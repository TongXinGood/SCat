#include "../include/Motion.h"
#include "../include/Theme.h"
#include <QWidget>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QGraphicsOpacityEffect>
#include <QPixmap>
#include <QLineF>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Motion
{
    static const int kSlideOffset = 16;     // 新消息从下面多远浮上来
    static const qreal kPopupFromScale = 0.6;   // 面板从多大开始放大

    QEasingCurve emphasizedDecelerate()
    {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.05, 0.7), QPointF(0.1, 1.0), QPointF(1.0, 1.0));
        return curve;
    }

    QEasingCurve emphasizedAccelerate()
    {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.3, 0.0), QPointF(0.8, 0.15), QPointF(1.0, 1.0));
        return curve;
    }

    QEasingCurve standard()
    {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.2, 0.0), QPointF(0.0, 1.0), QPointF(1.0, 1.0));
        return curve;
    }

    bool enabled()
    {
#ifdef Q_OS_WIN
        // 对应 Windows 设置 → 辅助功能 → 视觉效果 → "动画效果"
        BOOL on = TRUE;
        if (SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &on, 0))
            return on != FALSE;
#endif
        return true;
    }

    // ============ 水波纹 ============

    // 盖在目标控件上面的一层透明控件，只负责画波纹。
    // 鼠标事件直接穿透给下面的控件，所以按钮该怎么响应还怎么响应
    class RippleOverlay : public QWidget
    {
    public:
        RippleOverlay(QWidget* target, int radius, const QColor& color)
            : QWidget(target), target(target), radius(radius), color(color)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);
            setGeometry(target->rect());
            target->installEventFilter(this);
            show();
        }

        ~RippleOverlay() override
        {
            // 按钮被删的时候可能还有没淡完的波纹
            qDeleteAll(ripples);
        }

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (watched != target)
                return false;

            switch (event->type()) {
            case QEvent::Resize:
                setGeometry(target->rect());
                break;
            case QEvent::MouseButtonPress: {
                QMouseEvent* e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton && target->isEnabled())
                    press(e->position());
                break;
            }
            case QEvent::MouseButtonRelease:
            case QEvent::Leave:
            case QEvent::Hide:
                releaseAll();
                break;
            default:
                break;
            }
            return false;
        }

        void paintEvent(QPaintEvent*) override
        {
            if (ripples.isEmpty())
                return;

            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);

            QPainterPath clip;
            clip.addRoundedRect(QRectF(rect()), radius, radius);
            p.setClipPath(clip);

            for (const Ripple* r : ripples) {
                QColor c = color;
                c.setAlphaF(color.alphaF() * r->alpha);
                p.setBrush(c);
                p.drawEllipse(r->center, r->radius, r->radius);
            }
        }

    private:
        // 一圈波纹。按下时开始扩散，松开后慢慢淡出，淡完了删掉
        struct Ripple
        {
            QPointF center;
            qreal radius = 0;
            qreal alpha = 1;
            QVariantAnimation* fade = nullptr;
        };

        void press(const QPointF& pos)
        {
            raise();       // 目标控件后来又加了子控件的话，波纹要保持在最上面

            // 扩散到能盖住离按下位置最远的那个角
            qreal maxRadius = 0;
            for (const QPointF& corner : { QPointF(0, 0), QPointF(width(), 0),
                    QPointF(0, height()), QPointF(width(), height()) })
                maxRadius = qMax(maxRadius, QLineF(pos, corner).length());

            Ripple* r = new Ripple;
            r->center = pos;
            ripples.append(r);

            if (!Motion::enabled()) {
                r->radius = maxRadius;
                update();
                return;
            }

            QVariantAnimation* grow = new QVariantAnimation(this);
            grow->setDuration(kLong);
            grow->setStartValue(0.0);
            grow->setEndValue(maxRadius);
            grow->setEasingCurve(standard());
            connect(grow, &QVariantAnimation::valueChanged, this, [this, r](const QVariant& v) {
                if (ripples.contains(r)) {
                    r->radius = v.toReal();
                    update();
                }
                });
            grow->start(QAbstractAnimation::DeleteWhenStopped);
        }

        void releaseAll()
        {
            for (Ripple* r : ripples) {
                if (r->fade)
                    continue;      // 已经在淡出了

                r->fade = new QVariantAnimation(this);
                r->fade->setDuration(Motion::enabled() ? kMedium : 0);
                r->fade->setStartValue(1.0);
                r->fade->setEndValue(0.0);
                r->fade->setEasingCurve(standard());
                connect(r->fade, &QVariantAnimation::valueChanged, this, [this, r](const QVariant& v) {
                    r->alpha = v.toReal();
                    update();
                    });
                connect(r->fade, &QVariantAnimation::finished, this, [this, r]() {
                    ripples.removeOne(r);
                    delete r;
                    update();
                    });
                r->fade->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }

    private:
        QWidget* target;
        int radius;
        QColor color;
        QList<Ripple*> ripples;
    };

    void addRipple(QWidget* target, int radius, RippleTone tone)
    {
        if (!target)
            return;

        // 深色按钮上压一层半透明白；浅色底上压一层半透明深紫灰，深色模式下也换成白
        QColor color;
        if (tone == OnPrimary)
            color = QColor(255, 255, 255, 50);
        else
            color = Theme::isDark() ? QColor(255, 255, 255, 30) : QColor(32, 32, 46, 28);

        new RippleOverlay(target, radius, color);
    }

    // ============ 新消息上浮 ============

    void slideUpIn(QWidget* widget)
    {
        if (!widget || !widget->layout() || !Motion::enabled())
            return;

        QLayout* layout = widget->layout();
        QMargins m = layout->contentsMargins();

        // 上边距加大、下边距减小，总高度不变，列表不用重新算行高
        int offset = qMin(kSlideOffset, m.bottom());

        // 已经挂了别的效果（比如阴影）就只做位移，一个控件只能挂一个效果
        QGraphicsOpacityEffect* fade = nullptr;
        if (!widget->graphicsEffect()) {
            fade = new QGraphicsOpacityEffect(widget);
            fade->setOpacity(0.0);
            widget->setGraphicsEffect(fade);
        }

        // 位置走 Material 的减速曲线；透明度按时间匀速，前一半就完全显出来
        QEasingCurve curve = emphasizedDecelerate();
        auto apply = [layout, m, offset, fade, curve](qreal t) {
            int shift = qRound(offset * (1.0 - curve.valueForProgress(t)));
            layout->setContentsMargins(m.left(), m.top() + shift, m.right(), m.bottom() - shift);
            if (fade)
                fade->setOpacity(qMin(1.0, t * 2.0));
            };
        apply(0.0);

        QVariantAnimation* anim = new QVariantAnimation(widget);
        anim->setDuration(kLong);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        QObject::connect(anim, &QVariantAnimation::valueChanged, widget, [apply](const QVariant& v) {
            apply(v.toReal());
            });
        QObject::connect(anim, &QVariantAnimation::finished, widget, [widget, fade]() {
            // 动画完了把透明度效果拿掉，它会让控件每次重绘都多走一遍离屏渲染
            if (fade && widget->graphicsEffect() == fade)
                widget->setGraphicsEffect(nullptr);
            });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // ============ 弹出面板展开 ============

    // 动画期间代替真面板显示的截图：围绕 origin 从小放大，同时淡入
    class PopupSnapshot : public QWidget
    {
    public:
        PopupSnapshot(QWidget* popup, const QPixmap& shot, const QPoint& origin)
            : QWidget(popup), shot(shot), origin(origin)
        {
            setObjectName("MotionSnapshot");
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setGeometry(popup->rect());
        }

        void setProgress(qreal t)
        {
            progress = t;
            update();
        }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::SmoothPixmapTransform);

            // 透明度按时间匀速，前四成时间就完全显出来；大小走 Material 的减速曲线
            p.setOpacity(qMin(1.0, progress * 2.5));

            qreal eased = emphasizedDecelerate().valueForProgress(progress);
            qreal s = kPopupFromScale + (1.0 - kPopupFromScale) * eased;
            p.translate(origin);
            p.scale(s, s);
            p.translate(-origin);
            p.drawPixmap(0, 0, shot);
        }

    private:
        QPixmap shot;
        QPoint origin;
        qreal progress = 0;
    };

    void popupOpen(QWidget* popup, QWidget* content, const QPoint& origin)
    {
        if (!popup || !content)
            return;

        // 上一次的动画还没放完又打开了，先收拾掉
        if (QWidget* old = popup->findChild<QWidget*>("MotionSnapshot", Qt::FindDirectChildrenOnly)) {
            old->deleteLater();
            content->show();
        }

        if (!Motion::enabled()) {
            popup->show();
            return;
        }

        // 先以全透明显示出来，让布局排好，才能截到完整的图
        popup->setWindowOpacity(0.0);
        popup->show();

        qreal dpr = popup->devicePixelRatioF();
        QPixmap shot(popup->size() * dpr);
        shot.setDevicePixelRatio(dpr);
        shot.fill(Qt::transparent);
        popup->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);

        content->hide();
        PopupSnapshot* snap = new PopupSnapshot(popup, shot, origin);
        snap->show();
        popup->setWindowOpacity(1.0);

        QVariantAnimation* anim = new QVariantAnimation(snap);
        anim->setDuration(kLong);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        QObject::connect(anim, &QVariantAnimation::valueChanged, snap, [snap](const QVariant& v) {
            snap->setProgress(v.toReal());
            });
        QObject::connect(anim, &QVariantAnimation::finished, snap, [snap, content]() {
            content->show();
            snap->deleteLater();
            });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}