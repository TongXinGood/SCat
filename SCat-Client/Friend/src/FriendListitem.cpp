#include "../include/FriendListitem.h"
#include <QFontMetrics>
#include <QDateTime>

static const int kAvatarSize = 45;

// 列表里的时间要短：今天只显示几点，昨天显示"昨天"，
// 一周内显示星期几，再早就只显示日期
static QString formatListTime(qint64 ms)
{
    QDateTime t = QDateTime::fromMSecsSinceEpoch(ms);
    QDate day = t.date();
    QDate today = QDate::currentDate();

    if (day == today)
        return t.toString("HH:mm");

    if (day == today.addDays(-1))
        return QStringLiteral("昨天");

    if (day > today.addDays(-7)) {
        static const char* kWeekdays[] = { "", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };
        return QString::fromUtf8(kWeekdays[day.dayOfWeek()]);
    }

    if (day.year() == today.year())
        return QString("%1/%2").arg(day.month()).arg(day.day());

    return QString("%1/%2/%3").arg(day.year() % 100).arg(day.month()).arg(day.day());
}

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
    lbAvatar->setFixedSize(kAvatarSize, kAvatarSize);
    lbAvatar->setPixmap(avatar);
    // 图已经是裁好的圆形：不能开 setScaledContents，背景也必须透明

    textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    // 第一行：名字 ………… 时间
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(8);

    lbName = new QLabel(name, this);
    lbName->setObjectName("ItemName");
    // 名字太长时让它被压窄，不能把右边的时间挤出去
    lbName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    lbTime = new QLabel(this);
    lbTime->setObjectName("ItemTime");
    lbTime->hide();

    topRow->addWidget(lbName, 1);
    topRow->addWidget(lbTime);

    // 第二行：最后一条消息 ………… 未读红点
    QHBoxLayout* bottomRow = new QHBoxLayout();
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(8);

    lbLastMsg = new EmojiLabel(lastMsg, this);
    lbLastMsg->setObjectName("ItemLastMsg");

    lbUnread = new QLabel(this);
    lbUnread->setObjectName("UnreadBadge");
    lbUnread->setAlignment(Qt::AlignCenter);
    lbUnread->setFixedHeight(18);
    lbUnread->hide();

    bottomRow->addWidget(lbLastMsg, 1);
    bottomRow->addWidget(lbUnread);

    textLayout->addStretch(1);
    textLayout->addLayout(topRow);
    textLayout->addLayout(bottomRow);
    textLayout->addStretch(1);

    mainLayout->addWidget(lbAvatar);
    mainLayout->addLayout(textLayout, 1);

    this->setStyleSheet(R"(
        #FriendItem   { background-color: transparent; }
        #ItemAvatar   { background: transparent; border: none; }
        #ItemName {
            font-weight: bold; font-size: 15px; color: #1A1A1A;
            background-color: transparent; border: none;
        }
        #ItemTime {
            font-size: 11px; color: #A3A1AB;
            background-color: transparent; border: none;
        }
        #ItemLastMsg {
            font-size: 13px; color: #757575;
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

void FriendListItem::setTime(qint64 time)
{
    if (time <= 0) {
        lbTime->hide();
        return;
    }

    lbTime->setText(formatListTime(time));
    lbTime->show();
}