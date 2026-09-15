#include "../include/FriendListitem.h"
#include <QFontMetrics>

FriendListItem::FriendListItem(const QString& id, const QPixmap& avatar, const QString& name,
    const QString& lastMsg, QWidget* parent)
    : QWidget(parent), friendId(id), friendName(name)
{
    initUI(avatar, name, lastMsg);
}

FriendListItem::~FriendListItem()
{
}

void FriendListItem::initUI(const QPixmap& avatar, const QString& name, const QString& lastMsg)
{
    // 自身背景透明，否则会盖住 QListWidget 的选中高亮
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setObjectName("FriendItem");

    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(12);

    lbAvatar = new QLabel(this);
    lbAvatar->setObjectName("ItemAvatar");
    lbAvatar->setFixedSize(45, 45);
    lbAvatar->setPixmap(avatar);
    // 图已经是 45x45 的圆形：不能开 setScaledContents，背景也必须透明

    textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    lbName = new QLabel(name, this);
    lbName->setObjectName("ItemName");

    lbLastMsg = new QLabel(lastMsg, this);
    lbLastMsg->setObjectName("ItemLastMsg");

    textLayout->addWidget(lbName);
    textLayout->addWidget(lbLastMsg);
    textLayout->addStretch(1);

    // ★ 未读数红点。这一段之前漏了，导致 lbUnread 是野指针，
    // 一调 setUnread 就崩。纯色背景没有 pixmap，所以 QSS 的
    // border-radius 是生效的，不用像头像那样自己画圆
    lbUnread = new QLabel(this);
    lbUnread->setObjectName("UnreadBadge");
    lbUnread->setAlignment(Qt::AlignCenter);
    lbUnread->setFixedHeight(18);
    lbUnread->hide();

    mainLayout->addWidget(lbAvatar);
    mainLayout->addLayout(textLayout, 1);       // 占满中间，把红点挤到最右边
    mainLayout->addWidget(lbUnread, 0, Qt::AlignVCenter);

    this->setStyleSheet(R"(
        #FriendItem   { background-color: transparent; }
        #ItemAvatar   { background: transparent; border: none; }
        #ItemName {
            font-weight: bold; font-size: 15px; color: #1A1A1A;
            background-color: transparent; border: none;
        }
        #ItemLastMsg {
            font-size: 13px; color: #757575;
            background-color: transparent; border: none;
        }
        #UnreadBadge {
            background-color: #FF4D4F;
            color: #FFFFFF;
            border: none;
            border-radius: 9px;
            font-size: 11px;
            font-weight: bold;
        }
    )");
}

QString FriendListItem::getFriendId() const
{
    return friendId;
}

QString FriendListItem::getFriendName() const
{
    return friendName;
}

void FriendListItem::setLastMessage(const QString& msg)
{
    lbLastMsg->setText(msg);
}

void FriendListItem::setAvatar(const QPixmap& avatar)
{
    lbAvatar->setPixmap(avatar);
}

void FriendListItem::setUnread(int count)
{
    if (count <= 0) {
        lbUnread->hide();
        return;
    }

    QString text = count > 99 ? QStringLiteral("99+") : QString::number(count);
    lbUnread->setText(text);

    // 一位数是正圆（18x18），两位以上自动拉成胶囊
    int w = qMax(18, QFontMetrics(lbUnread->font()).horizontalAdvance(text) + 10);
    lbUnread->setFixedWidth(w);

    lbUnread->show();
}