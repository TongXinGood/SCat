#include "../include/FriendList.h"
#include "../include/FriendListitem.h"
#include <QIcon>
#include <QCursor>

FriendList::FriendList(QWidget* parent) : QWidget(parent)
{
    initUI();
    initConnect();
}

FriendList::~FriendList()
{
}

void FriendList::initUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 10, 0, 0);
    mainLayout->setSpacing(10);

    // 搜索区域外部容器
    searchContainer = new QWidget(this);
    searchContainer->setFixedHeight(40);
    searchContainer->setAttribute(Qt::WA_StyledBackground, true);
    searchContainer->setObjectName("SearchContainer");
    searchContainer->setStyleSheet(
        "#SearchContainer { "
        "   background-color: #FFFFFF; "
        "   border-radius: 6px; "
        "   border: 1px solid #E5E5E5; "
        "}"
    );

    searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(10, 0, 9, 0);
    searchLayout->setSpacing(8);

    lbSearchIcon = new QLabel(searchContainer);
    lbSearchIcon->setFixedSize(18, 18);
    lbSearchIcon->setScaledContents(true);
    lbSearchIcon->setPixmap(QPixmap(":/Resource/icon/search.png"));
    lbSearchIcon->setStyleSheet("border: none; background: transparent;");

    editSearch = new QLineEdit(searchContainer);
    editSearch->setPlaceholderText("Search chats");
    editSearch->setStyleSheet(
        "QLineEdit {"
        "   border: none;"
        "   background: transparent;"
        "   font-size: 13px;"
        "   color: #333333;"
        "}"
    );

    btnAdd = new QPushButton(searchContainer);
    btnAdd->setFixedSize(24, 24);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setIcon(QIcon(":/Resource/icon/add.png"));
    btnAdd->setIconSize(QSize(17, 17));
    btnAdd->setStyleSheet(
        "QPushButton { "
        "   border: none; "
        "   background: transparent; "
        "} "
        "QPushButton:hover { "
        "   background: #F5F5F5; "
        "   border-radius: 12px; "
        "}"
    );

    searchLayout->addWidget(lbSearchIcon);
    searchLayout->addWidget(editSearch);
    searchLayout->addWidget(btnAdd);

    // 列表主体
    listWidget = new QListWidget(this);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 强制隐藏滚动条，鼠标滚轮照样能滚
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    listWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: transparent;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   border-bottom: 1px solid #F0F0F0;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #F8F9FA;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #EFEBFA;"
        "   border: none;"
        "}"
    );
    QHBoxLayout* searchRow = new QHBoxLayout();
    searchRow->setContentsMargins(10, 0, 10, 0);
    searchRow->setSpacing(0);
    searchRow->addWidget(searchContainer);

    mainLayout->addLayout(searchRow);
    mainLayout->addWidget(listWidget);
}

void FriendList::initConnect()
{
    connect(editSearch, &QLineEdit::textChanged, this, &FriendList::onSearchTextChanged);
    connect(listWidget, &QListWidget::itemClicked, this, &FriendList::onItemClicked);
    connect(btnAdd, &QPushButton::clicked, this, &FriendList::onAddClicked);
}

void FriendList::addFriendItem(const QString& id, const QPixmap& avatar,
    const QString& name, const QString& lastMsg)
{
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    FriendListItem* customWidget = new FriendListItem(id, avatar, name, lastMsg, this);

    item->setSizeHint(QSize(listWidget->width(), 65));
    listWidget->setItemWidget(item, customWidget);
}

void FriendList::clearFriends()
{
    // clear() 会删掉所有 item，setItemWidget 设进去的 FriendListItem
    // 归视图所有，也会跟着一起销毁，不会泄漏
    listWidget->clear();
    currentId.clear();
}

void FriendList::clearSelection()
{
    listWidget->clearSelection();
    listWidget->setCurrentItem(nullptr);
    currentId.clear();
}

void FriendList::updateLastMessage(const QString& id, const QString& msg)
{
    for (int i = 0; i < listWidget->count(); ++i) {
        FriendListItem* item = qobject_cast<FriendListItem*>(
            listWidget->itemWidget(listWidget->item(i)));

        if (item && item->getFriendId() == id) {
            item->setLastMessage(msg);
            return;
        }
    }
}

void FriendList::updateAvatar(const QString& id, const QPixmap& avatar)
{
    for (int i = 0; i < listWidget->count(); ++i) {
        FriendListItem* item = qobject_cast<FriendListItem*>(
            listWidget->itemWidget(listWidget->item(i)));

        if (item && item->getFriendId() == id) {
            item->setAvatar(avatar);
            return;
        }
    }
}

void FriendList::setUnread(const QString& id, int count)
{
    for (int i = 0; i < listWidget->count(); ++i) {
        FriendListItem* item = qobject_cast<FriendListItem*>(
            listWidget->itemWidget(listWidget->item(i)));

        if (item && item->getFriendId() == id) {
            item->setUnread(count);
            return;
        }
    }
}

void FriendList::selectFriend(const QString& id)
{
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem* item = listWidget->item(i);
        FriendListItem* widget = qobject_cast<FriendListItem*>(
            listWidget->itemWidget(item));

        if (!widget || widget->getFriendId() != id)
            continue;

        listWidget->setCurrentItem(item);
        currentId = id;

        // 手动发一次信号，走跟鼠标点击完全一样的流程
        emit sendFriendSelected(widget->getFriendId(), widget->getFriendName());
        return;
    }
}

void FriendList::onSearchTextChanged(const QString& text)
{
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem* item = listWidget->item(i);
        FriendListItem* customWidget = qobject_cast<FriendListItem*>(
            listWidget->itemWidget(item));

        if (customWidget) {
            bool isMatch = customWidget->getFriendName().contains(text, Qt::CaseInsensitive);
            item->setHidden(!isMatch);
        }
    }
}

void FriendList::onItemClicked(QListWidgetItem* item)
{
    FriendListItem* customWidget = qobject_cast<FriendListItem*>(listWidget->itemWidget(item));
    if (!customWidget)
        return;

    // 再点一次已经选中的那个人 = 取消选中，右边回到默认页
    if (currentId == customWidget->getFriendId()) {
        listWidget->clearSelection();
        listWidget->setCurrentItem(nullptr);
        currentId.clear();
        emit sendFriendUnselected();
        return;
    }

    currentId = customWidget->getFriendId();
    emit sendFriendSelected(customWidget->getFriendId(), customWidget->getFriendName());
}

void FriendList::onAddClicked()
{
    emit sendAddFriendClicked();
}