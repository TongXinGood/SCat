#include "../include/RequestWindow.h"
#include "../include/RequestItem.h"
#include <QJsonObject>
#include <QDebug>

RequestWindow::RequestWindow(QWidget* parent) : NoFrame(parent)
{
    NoFrame::Frameconfig config;
    config.background = QColor(255, 255, 255);
    config.titlebarcolor = QColor(255, 255, 255);
    config.borderRadius = 15;
    config.defaultsize = QSize(400, 560);
    config.titlebarheight = 40;
    config.showlogo = false;
    config.showtext = false;
    config.showmin = false;
    config.showmax = false;
    config.showclose = true;
    config.btnsize = QSize(24, 24);
    this->setFrameconfig(config);

    this->setFixedSize(400, 560);

    initUI();
    refreshEmptyHint();
}

RequestWindow::~RequestWindow()
{
}

QString RequestWindow::avatarPath(const QString& avatar)
{
    if (avatar.isEmpty())
        return ":/Resource/icon/head.png";
    return ":/Resource/icon/" + avatar;
}

void RequestWindow::initUI()
{
    content = new QWidget(this);
    content->setObjectName("RequestContent");

    mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(24, 8, 24, 24);
    mainLayout->setSpacing(14);

    lbTitle = new QLabel("好友申请", content);
    lbTitle->setObjectName("ReqTitle");

    listWidget = new QListWidget(content);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lbEmpty = new QLabel("暂无新的好友申请", content);
    lbEmpty->setObjectName("ReqEmpty");
    lbEmpty->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(lbTitle);
    mainLayout->addWidget(listWidget, 1);
    mainLayout->addWidget(lbEmpty, 1);

    this->setMainWindow(content);

    content->setStyleSheet(R"(
        #RequestContent { background-color: #FFFFFF; }

        #ReqTitle {
            font-size: 18px; font-weight: bold; color: #1A1A1A;
        }
        #ReqEmpty {
            font-size: 13px; color: #999999;
        }

        QListWidget {
            background-color: transparent;
            border: none;
            outline: none;
        }
        QListWidget::item {
            border-bottom: 1px solid #F2F2F2;
        }
        QListWidget::item:hover, QListWidget::item:selected {
            background-color: transparent;
        }
    )");
}

void RequestWindow::refreshEmptyHint()
{
    bool empty = (listWidget->count() == 0);
    listWidget->setVisible(!empty);
    lbEmpty->setVisible(empty);
}

int RequestWindow::indexOf(const QString& username) const
{
    for (int i = 0; i < listWidget->count(); ++i) {
        RequestItem* item = qobject_cast<RequestItem*>(
            listWidget->itemWidget(listWidget->item(i)));
        if (item && item->username() == username)
            return i;
    }
    return -1;
}

void RequestWindow::appendItem(const QString& username, const QString& nickname,
    const QString& avatar)
{
    // 已经有这个人的申请了就不重复加（比如推送和主动拉取撞在一起）
    if (indexOf(username) >= 0)
        return;

    QListWidgetItem* item = new QListWidgetItem(listWidget);
    RequestItem* widget = new RequestItem(username, nickname, avatarPath(avatar), this);

    item->setSizeHint(QSize(listWidget->width(), 68));
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    listWidget->setItemWidget(item, widget);

    connect(widget, &RequestItem::sendAccept, this, [this](const QString& name) {
        emit sendHandleRequest(name, HANDLE_ACCEPT);
        });
    connect(widget, &RequestItem::sendReject, this, [this](const QString& name) {
        emit sendHandleRequest(name, HANDLE_REJECT);
        });
}

void RequestWindow::setRequests(const QJsonArray& requests)
{
    listWidget->clear();

    for (const QJsonValue& value : requests) {
        QJsonObject obj = value.toObject();
        appendItem(obj["username"].toString(),
            obj["nickname"].toString(),
            obj["avatar"].toString());
    }

    refreshEmptyHint();
}

void RequestWindow::addRequest(const QString& username, const QString& nickname,
    const QString& avatar)
{
    appendItem(username, nickname, avatar);
    refreshEmptyHint();
}

void RequestWindow::removeRequest(const QString& username)
{
    int row = indexOf(username);
    if (row < 0)
        return;

    // takeItem 把 item 摘出来，delete 时挂在上面的 RequestItem 也会一起销毁
    delete listWidget->takeItem(row);
    refreshEmptyHint();
}

void RequestWindow::setItemBusy(const QString& username, bool busy)
{
    int row = indexOf(username);
    if (row < 0)
        return;

    RequestItem* item = qobject_cast<RequestItem*>(
        listWidget->itemWidget(listWidget->item(row)));
    if (item)
        item->setBusy(busy);
}