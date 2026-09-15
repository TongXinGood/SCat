#include "../include/FriendListitem.h"

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
    // 自身背景设成透明，否则会盖住 QListWidget 的选中高亮
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("QWidget { background-color: transparent; }");

    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(15);

    lbAvatar = new QLabel(this);
    lbAvatar->setFixedSize(45, 45);
    lbAvatar->setPixmap(avatar);
    // 传进来的图已经是 45x45 的圆形了，所以：
    //   1. 不能开 setScaledContents，会把图拉变形
    //   2. 背景必须透明，圆形图四角是透明的，底下有色块会露出方角
    lbAvatar->setStyleSheet("background: transparent; border: none;");

    textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    lbName = new QLabel(name, this);
    lbName->setStyleSheet("font-weight: bold; font-size: 15px; color: #1A1A1A; "
        "background-color: transparent; border: none;");

    lbLastMsg = new QLabel(lastMsg, this);
    lbLastMsg->setStyleSheet("font-size: 13px; color: #757575; "
        "background-color: transparent; border: none;");

    textLayout->addWidget(lbName);
    textLayout->addWidget(lbLastMsg);
    textLayout->addStretch(1);

    mainLayout->addWidget(lbAvatar);
    mainLayout->addLayout(textLayout);
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