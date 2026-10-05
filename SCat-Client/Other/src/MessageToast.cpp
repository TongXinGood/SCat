#include "../include/MessageToast.h"
#include "../include/AvatarUtils.h"
#include "../include/Theme.h"
#include "../../Chat/include/EmojiLabel.h"
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QMouseEvent>

static const int kShadow = 14;          // 卡片外留给阴影的透明边距
static const int kCardWidth = 320;
static const int kAvatarSize = 44;
static const int kBadgeHeight = 18;
static const int kLifeMs = 5000;        // 停留多久自动消失
static const int kSlideOffset = 24;     // 弹出时从下面多远滑上来

// 配色跟表情面板、文件气泡是一套。以后做暗色模式只要换这一段
static const char* kToastStyle = R"(
    QFrame#ToastCard {
        background-color: #FFFFFF;
        border: 1px solid #ECE9F2;
        border-radius: 14px;
    }
    QLabel#ToastName {
        font-size: 14px; font-weight: bold; color: #1A1A1A;
        background: transparent;
    }
    QLabel#ToastTime {
        font-size: 11px; color: #9E9E9E;
        background: transparent;
    }
    #ToastText {
        font-size: 13px; color: #757575;
    }
    QLabel#ToastBadge {
        background-color: #FF4D4F; color: #FFFFFF;
        border: 2px solid #FFFFFF; border-radius: 9px;
        font-size: 10px; font-weight: bold;
        padding: 0px 4px;
    }
    QPushButton#ToastClose {
        background: transparent; border: none; border-radius: 10px;
        color: #9E9E9E; font-size: 14px;
    }
    QPushButton#ToastClose:hover { background-color: #F4F1FA; color: #20202E; }
)";

MessageToast::MessageToast(QWidget* parent)
    : QWidget(parent), msgCount(0), dismissing(false)
{
    // Tool：不在任务栏上占位置；StaysOnTop：盖在别的软件上面；
    // 两个"不抢焦点"：弹出来的时候不能打断用户正在别处打字
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
        | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    lifeTimer.setSingleShot(true);
    lifeTimer.setInterval(kLifeMs);
    connect(&lifeTimer, &QTimer::timeout, this, &MessageToast::dismiss);

    initUi();
}

void MessageToast::initUi()
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(kShadow, kShadow, kShadow, kShadow);

    card = new QFrame(this);
    card->setObjectName("ToastCard");
    card->setFixedWidth(kCardWidth);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(Theme::css(kToastStyle));

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 6);
    // 深色底上淡阴影看不出来，换成更深更浓的黑
    shadow->setColor(Theme::isDark() ? QColor(0, 0, 0, 130) : QColor(32, 32, 46, 50));
    card->setGraphicsEffect(shadow);

    outer->addWidget(card);

    QHBoxLayout* row = new QHBoxLayout(card);
    row->setContentsMargins(14, 14, 16, 14);
    row->setSpacing(12);

    // 头像和红点叠在一起，不进布局，自己摆位置。
    // 盒子比头像宽一点、高一点，给右上角的红点留地方
    QWidget* avatarBox = new QWidget(card);
    avatarBox->setFixedSize(kAvatarSize + 6, kAvatarSize + 4);

    lbAvatar = new QLabel(avatarBox);
    lbAvatar->setGeometry(0, 4, kAvatarSize, kAvatarSize);

    lbBadge = new QLabel(avatarBox);
    lbBadge->setObjectName("ToastBadge");
    lbBadge->setAlignment(Qt::AlignCenter);
    lbBadge->hide();

    QVBoxLayout* textCol = new QVBoxLayout();
    textCol->setContentsMargins(0, 2, 0, 2);
    textCol->setSpacing(4);

    QHBoxLayout* top = new QHBoxLayout();
    top->setSpacing(6);

    lbName = new QLabel(card);
    lbName->setObjectName("ToastName");

    // 右上角平时显示"刚刚"，鼠标移上来换成关闭按钮，两个占同一个位置
    lbTime = new QLabel("刚刚", card);
    lbTime->setObjectName("ToastTime");

    btnClose = new QPushButton("×", card);
    btnClose->setObjectName("ToastClose");
    btnClose->setFixedSize(20, 20);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setFocusPolicy(Qt::NoFocus);
    btnClose->hide();
    connect(btnClose, &QPushButton::clicked, this, &MessageToast::dismiss);

    top->addWidget(lbName, 1);
    top->addWidget(lbTime);
    top->addWidget(btnClose);

    lbText = new EmojiLabel(QString(), card);
    lbText->setObjectName("ToastText");

    textCol->addLayout(top);
    textCol->addWidget(lbText);

    row->addWidget(avatarBox, 0, Qt::AlignVCenter);
    row->addLayout(textCol, 1);
}

void MessageToast::setMessage(const QString& peer, const QString& name,
    const QPixmap& avatar, const QString& text, int count)
{
    peerId = peer;
    msgCount = count;

    // 按屏幕缩放准备头像，150%、200% 的屏上才不糊
    qreal dpr = devicePixelRatioF();
    QPixmap round = AvatarUtils::round(avatar, qRound(kAvatarSize * dpr));
    round.setDevicePixelRatio(dpr);
    lbAvatar->setPixmap(round);

    lbName->setText(name);
    lbText->setText(text);

    if (count > 1) {
        lbBadge->setText(count > 99 ? QStringLiteral("99+") : QString::number(count));

        // 一位数是正圆，两位以上拉成胶囊，贴着头像框的右上角
        int w = qMax(kBadgeHeight, lbBadge->sizeHint().width());
        lbBadge->setGeometry(kAvatarSize + 6 - w, 0, w, kBadgeHeight);
        lbBadge->show();
    }
    else {
        lbBadge->hide();
    }

    adjustSize();
}

void MessageToast::popup(const QPoint& pos)
{
    move(pos.x(), pos.y() + kSlideOffset);
    setWindowOpacity(0.0);
    show();

    // 滑动和淡入分开跑，不能放进同一个动画组：滑到一半又来一条新消息时，
    // moveTo 会启动新的位置动画，Qt 会把旧的位置动画停掉，
    // 要是在一个组里，淡入也跟着被停，弹窗就一直是透明的
    QPropertyAnimation* slide = new QPropertyAnimation(this, "pos", this);
    slide->setDuration(260);
    slide->setEndValue(pos);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->start(QAbstractAnimation::DeleteWhenStopped);

    QPropertyAnimation* fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(200);
    fade->setEndValue(1.0);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    lifeTimer.start();
}

void MessageToast::moveTo(const QPoint& pos)
{
    if (dismissing || this->pos() == pos)
        return;

    QPropertyAnimation* slide = new QPropertyAnimation(this, "pos", this);
    slide->setDuration(200);
    slide->setEndValue(pos);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->start(QAbstractAnimation::DeleteWhenStopped);
}

void MessageToast::dismiss()
{
    if (dismissing)
        return;

    dismissing = true;
    lifeTimer.stop();

    // 先告诉 Notification 把自己从队列里拿掉，别的弹窗好马上补位
    emit closed(this);

    QPropertyAnimation* fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(180);
    fade->setEndValue(0.0);
    connect(fade, &QPropertyAnimation::finished, this, &QObject::deleteLater);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

void MessageToast::restartTimer()
{
    // 鼠标正停在上面就先不计时，等移开再说
    if (!underMouse())
        lifeTimer.start();
}

void MessageToast::enterEvent(QEnterEvent* event)
{
    // 用户在看，别让它自己消失
    lifeTimer.stop();
    lbTime->hide();
    btnClose->show();
    QWidget::enterEvent(event);
}

void MessageToast::leaveEvent(QEvent* event)
{
    if (!dismissing)
        lifeTimer.start();

    btnClose->hide();
    lbTime->show();
    QWidget::leaveEvent(event);
}

void MessageToast::mouseReleaseEvent(QMouseEvent* event)
{
    // 只认点在卡片上的左键，旁边透明的阴影区域不算
    if (event->button() == Qt::LeftButton && card->geometry().contains(event->pos())) {
        emit clicked(peerId);
        dismiss();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}