#include "../include/EmojiPicker.h"
#include "../../Other/include/UserSession.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QSettings>

static const int kColumns = 8;       // 每行几个表情
static const int kCellSize = 36;     // 每一格的大小，悬停时浅紫底色就是这么大
static const int kIconSize = 26;     // 格子里表情的大小，四周留点空才不挤
static const int kCellGap = 2;
static const int kCardHeight = 300;
static const int kShadow = 12;       // 卡片外留给阴影的透明边距
static const int kAnchorGap = 6;     // 卡片底边跟表情按钮之间的空隙
static const int kRecentMax = 8;     // 最近使用最多记几个，正好一行

// 配色跟右键菜单、文件气泡是一套：白底、浅紫边框、悬停浅紫
static const char* kPickerStyle = R"(
    QWidget#EmojiCard {
        background-color: #FFFFFF;
        border: 1px solid #ECE9F2;
        border-radius: 12px;
    }
    QWidget#EmojiContent, QScrollArea#EmojiScroll, QWidget#EmojiScrollViewport {
        background: transparent;
        border: none;
    }
    QLabel#EmojiTitle {
        background: transparent;
        color: #7E7A8C;
        font-size: 12px;
        padding-left: 4px;
    }
    QPushButton#EmojiCell {
        background: transparent;
        border: none;
        border-radius: 8px;
    }
    QPushButton#EmojiCell:hover   { background-color: #F4F1FA; }
    QPushButton#EmojiCell:pressed { background-color: #E7E1F5; }

    QScrollBar:vertical {
        background: transparent;
        width: 6px;
        margin: 4px 0px 4px 0px;
    }
    QScrollBar::handle:vertical {
        background-color: #DCD6E6;
        border-radius: 3px;
        min-height: 30px;
    }
    QScrollBar::handle:vertical:hover { background-color: #C9C1D8; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
)";

EmojiPicker::EmojiPicker(QWidget* parent) : QWidget(parent), contentLayout(nullptr), recentSection(nullptr)
{
    // Popup：点窗口外面任何地方 Qt 会自动把它关掉，按 Esc 也关。
    // 圆角配透明背景、关掉系统方形阴影，跟 ChatInputEdit 的右键菜单是一个道理
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 面板开着的时候再点一下表情按钮，这一下只用来关面板。
    // 不加的话 Qt 会把这次点击转发给按钮，面板关了又立刻弹出来
    setAttribute(Qt::WA_NoMouseReplay);

    initUi();
}

void EmojiPicker::initUi()
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(kShadow, kShadow, kShadow, kShadow);

    card = new QWidget(this);
    card->setObjectName("EmojiCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(kPickerStyle);

    // 阴影很淡，只是让面板从聊天背景上"浮"起来一点
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(32, 32, 46, 40));
    card->setGraphicsEffect(shadow);

    outer->addWidget(card);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    // 右边距小一点，滚动条本身就占了 6px，看着左右才对称
    cardLayout->setContentsMargins(10, 10, 4, 10);

    scroll = new QScrollArea(card);
    scroll->setObjectName("EmojiScroll");
    scroll->viewport()->setObjectName("EmojiScrollViewport");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* content = new QWidget();
    content->setObjectName("EmojiContent");

    contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 4, 0);
    contentLayout->setSpacing(6);

    // 一个分类一段，从上往下排。现在只有黄脸，以后加了文件夹这里自动多出来
    for (const EmojiCategory& cat : EmojiManager::GetInstance().categories())
        contentLayout->addWidget(createSection(cat.name, cat.items));

    contentLayout->addStretch();

    scroll->setWidget(content);
    cardLayout->addWidget(scroll);

    // 宽度按格子算死：8 列格子 + 间距 + 滚动条 + 两边边距，
    // 这样每一行正好排满，右边不会空出半格
    int gridWidth = kColumns * kCellSize + (kColumns - 1) * kCellGap;
    int cardWidth = 10 + gridWidth + 4 + 6 + 4;
    setFixedSize(cardWidth + kShadow * 2, kCardHeight + kShadow * 2);
}

QWidget* EmojiPicker::createSection(const QString& title, const QList<EmojiItem>& items)
{
    QWidget* section = new QWidget();
    section->setObjectName("EmojiContent");

    QVBoxLayout* layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    QLabel* lbTitle = new QLabel(title, section);
    lbTitle->setObjectName("EmojiTitle");
    layout->addWidget(lbTitle);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(kCellGap);

    // 按屏幕缩放取图，150%、200% 的屏上才清楚
    qreal dpr = devicePixelRatioF();
    EmojiManager& em = EmojiManager::GetInstance();

    for (int i = 0; i < items.size(); ++i) {
        const EmojiItem& item = items.at(i);

        QPushButton* cell = new QPushButton(section);
        cell->setObjectName("EmojiCell");
        cell->setFixedSize(kCellSize, kCellSize);
        cell->setCursor(Qt::PointingHandCursor);
        cell->setFocusPolicy(Qt::NoFocus);
        cell->setIcon(QIcon(em.pixmap(item.id, kIconSize, dpr)));
        cell->setIconSize(QSize(kIconSize, kIconSize));

        QString text = item.text;
        connect(cell, &QPushButton::clicked, this, [this, item]() {
            // 先关再发：插入表情后输入框要拿焦点，面板还开着的话焦点抢不过去
            hide();

            // 这里只记下来，不立刻重建"最近使用"：点的可能正是那一段里的按钮，
            // 在它自己的点击信号里把它删掉会崩。下次打开面板时再重建
            addRecent(item.id);
            emit emojiSelected(item.text);
            });

        grid->addWidget(cell, i / kColumns, i % kColumns);
    }

    // 最后一行没排满时，格子靠左挤在一起，不要被拉开
    grid->setColumnStretch(kColumns, 1);

    layout->addLayout(grid);
    return section;
}

void EmojiPicker::popup(QWidget* anchor)
{
    if (!anchor)
        return;

    // 卡片右边对齐按钮右边，卡片底边在按钮上方 kAnchorGap 处。
    // 窗口比卡片多一圈阴影边距，所以要把 kShadow 补回来
    QPoint pos = anchor->mapToGlobal(QPoint(anchor->width(), 0));
    int x = pos.x() - width() + kShadow;
    int y = pos.y() - height() + kShadow - kAnchorGap;

    // 主窗口贴着屏幕边的时候别让面板跑出屏幕
    QRect screen = anchor->screen()->availableGeometry();
    x = qBound(screen.left(), x, screen.right() - width());
    y = qMax(screen.top(), y);

    refreshRecent();
    move(x, y);
    show();
}

void EmojiPicker::refreshRecent()
{
    if (recentSection) {
        contentLayout->removeWidget(recentSection);
        recentSection->deleteLater();
        recentSection = nullptr;
    }

    EmojiManager& em = EmojiManager::GetInstance();
    QList<EmojiItem> items;

    for (const QString& id : loadRecent()) {
        // 以后从资源里删掉了某个表情，老记录里的就跳过，不画空格子
        if (!em.contains(id))
            continue;

        EmojiItem item;
        item.id = id;
        item.text = em.textOf(id);
        items.append(item);
    }

    if (items.isEmpty())
        return;

    // 插在最前面，分类那几段都在它下面
    recentSection = createSection("最近使用", items);
    contentLayout->insertWidget(0, recentSection);
}

QStringList EmojiPicker::loadRecent() const
{
    // 按账号分开记：同一台电脑上登两个号，各用各的
    QSettings settings;
    return settings.value("emoji/recent/" + UserSession::GetInstance().username()).toStringList();
}

void EmojiPicker::addRecent(const QString& id)
{
    QStringList ids = loadRecent();

    // 已经在里面的先拿出来，再放到最前面，不会出现两个一样的
    ids.removeAll(id);
    ids.prepend(id);

    while (ids.size() > kRecentMax)
        ids.removeLast();

    QSettings settings;
    settings.setValue("emoji/recent/" + UserSession::GetInstance().username(), ids);
}