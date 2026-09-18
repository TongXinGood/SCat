#include "../include/ChatWindow.h"
#include <QDebug>
#include <QScrollBar>
#include <QAbstractItemView>

ChatWindow::ChatWindow(QWidget* parent) : QWidget(parent)
{
    // 开启 StyledBackground 属性，确保 QWidget 能正常渲染背景色
    this->setAttribute(Qt::WA_StyledBackground, true);

    // 设置全局对象名和背景色（淡紫色）
    this->setObjectName("ChatWindow");
    this->setStyleSheet("QWidget#ChatWindow { background-color: #F2F0F5; }");

    initUi();
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::initUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 顶部 Header
    initHeader();
    mainLayout->addWidget(headerWidget);

    // 2. 分割线
    QWidget* line = new QWidget(this);
    line->setFixedHeight(1);
    line->setStyleSheet("background-color: #E0E0E0;");
    mainLayout->addWidget(line);

    // 3. 中间消息列表 (淡紫色背景)
    initMsgList();
    mainLayout->addWidget(msgList, 1);

    // 4. 底部输入区
    initInput();
    mainLayout->addWidget(inputContainer);
}

void ChatWindow::initHeader()
{
    headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(70);
    headerWidget->setAttribute(Qt::WA_StyledBackground, true);
    headerWidget->setStyleSheet("background-color: #FFFFFF;");

    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(15);

    lbAvatar = new QLabel(headerWidget);
    lbAvatar->setFixedSize(45, 45);
    lbAvatar->setPixmap(QPixmap());
    lbAvatar->setStyleSheet("background: transparent; border: none;");

    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->setContentsMargins(0, 5, 0, 5);

    lbName = new QLabel("Helena Hills", headerWidget);
    lbName->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; border: none; background: transparent;");

    lbStatus = new QLabel("Active 20m ago", headerWidget);
    lbStatus->setStyleSheet("font-size: 12px; color: #999999; border: none; background: transparent;");

    infoLayout->addWidget(lbName);
    infoLayout->addWidget(lbStatus);
    infoLayout->addStretch();

    btnMore = new QPushButton(headerWidget);
    btnMore->setFixedSize(30, 30);
    btnMore->setCursor(Qt::PointingHandCursor);
    btnMore->setStyleSheet("QPushButton { border: none; background: transparent; }");
    btnMore->setIcon(QIcon(":/Resource/icon/more.png"));

    headerLayout->addWidget(lbAvatar);
    headerLayout->addLayout(infoLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(btnMore);
}

void ChatWindow::initMsgList()
{
    msgList = new QListWidget(this);
    msgList->setStyleSheet(
        "QListWidget {"
        "   background-color: transparent;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   background-color: transparent;"
        "   border: none;"
        "}"
        "QListWidget::item:selected, QListWidget::item:hover {"
        "   background-color: transparent;"
        "}"
    );
    msgList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    msgList->setSelectionMode(QAbstractItemView::NoSelection);
    msgList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ChatWindow::initInput()
{
    QWidget* inputOuter = new QWidget(this);
    inputOuter->setAttribute(Qt::WA_StyledBackground, true);
    inputOuter->setStyleSheet("background-color: transparent;");

    QVBoxLayout* outerLayout = new QVBoxLayout(inputOuter);
    outerLayout->setContentsMargins(20, 10, 20, 20);

    inputContainer = new QWidget(inputOuter);
    inputContainer->setFixedHeight(50);
    inputContainer->setAttribute(Qt::WA_StyledBackground, true);
    inputContainer->setObjectName("InputBar");
    inputContainer->setStyleSheet(
        "#InputBar { "
        "   background-color: #FFFFFF; "
        "   border-radius: 25px; "
        "   border: 1px solid #E5E5E5; "
        "}"
    );

    QHBoxLayout* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(20, 5, 15, 5);
    inputLayout->setSpacing(5);

    // 1. 输入框
    msgEdit = new QTextEdit(inputContainer);
    msgEdit->setPlaceholderText("Enter your message");
    msgEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgEdit->setStyleSheet(
        "QTextEdit {"
        "   border: none;"
        "   background: transparent;"
        "   font-size: 14px;"
        "   color: #333;"
        "   padding-top: 8px;"
        "}"
    );
    msgEdit->setFixedHeight(40);
    msgEdit->installEventFilter(this);

    // 2. 表情按钮 (QPushButton)
    btnmood = new QPushButton(inputContainer);
    btnmood->setFixedSize(24, 24);
    btnmood->setCursor(Qt::PointingHandCursor); // 显式设置鼠标手型
    btnmood->setIcon(QIcon(":/Resource/icon/mood.png"));
    btnmood->setIconSize(QSize(22, 22)); // 图标稍微小一点点，留点呼吸感
    btnmood->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #F5F5F5; border-radius: 12px; }");

    // 3. 文件按钮
    btnFile = new QPushButton(inputContainer);
    btnFile->setFixedSize(24, 24);
    btnFile->setCursor(Qt::PointingHandCursor); // 显式设置鼠标手型
    btnFile->setIcon(QIcon(":/Resource/icon/folder.png"));
    btnFile->setIconSize(QSize(20, 20));
    btnFile->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #F5F5F5; border-radius: 12px; }");

    inputLayout->addWidget(msgEdit);
    inputLayout->addWidget(btnmood);
    inputLayout->addWidget(btnFile);

    outerLayout->addWidget(inputContainer);
    this->inputContainer = inputOuter;

    connect(btnmood, &QPushButton::clicked, this, &ChatWindow::onMoodBtnClicked);
    connect(btnFile, &QPushButton::clicked, this, &ChatWindow::onFileBtnClicked);
}

QWidget* ChatWindow::createBubbleWidget(const QString& text, bool isSelf)
{
    QWidget* widget = new QWidget();
    widget->setStyleSheet("background-color: transparent;");
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel* bubble = new QLabel(text);
    bubble->setWordWrap(true);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(450);
    QFont font("Microsoft YaHei", 10);
    bubble->setFont(font);

    if (isSelf) {
        bubble->setStyleSheet("QLabel { background-color: #20202E; color: #FFFFFF; border-radius: 12px; padding: 12px 16px; }");
        layout->addStretch();
        layout->addWidget(bubble);
    }
    else {
        bubble->setStyleSheet("QLabel { background-color: #FFFFFF; color: #000000; border-radius: 12px; padding: 12px 16px; }");
        layout->addWidget(bubble);
        layout->addStretch();
    }
    return widget;
}

QWidget* ChatWindow::createFileBubbleWidget(const QString& fileName, const QString& fileSize, bool isSelf)
{
    QWidget* widget = new QWidget();
    widget->setStyleSheet("background-color: transparent;");
    QHBoxLayout* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(10, 5, 10, 5);

    QWidget* bubbleContent = new QWidget();
    bubbleContent->setAttribute(Qt::WA_StyledBackground, true);
    bubbleContent->setFixedSize(240, 75);

    QHBoxLayout* contentLayout = new QHBoxLayout(bubbleContent);
    contentLayout->setContentsMargins(15, 10, 15, 10);

    QLabel* iconLabel = new QLabel(bubbleContent);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setPixmap(QPixmap(":/Resource/icon/folder.png").scaled(40, 40));
    iconLabel->setStyleSheet("background: transparent; border: none;");

    QWidget* textRegion = new QWidget(bubbleContent);
    textRegion->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout* textLayout = new QVBoxLayout(textRegion);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    QLabel* nameLabel = new QLabel(fileName, textRegion);
    QFont nameFont("Microsoft YaHei", 9, QFont::Bold);
    nameLabel->setFont(nameFont);
    nameLabel->setText(QFontMetrics(nameFont).elidedText(fileName, Qt::ElideMiddle, 140));
    nameLabel->setStyleSheet("border: none; background: transparent;");

    QLabel* sizeLabel = new QLabel(fileSize, textRegion);
    sizeLabel->setStyleSheet("border: none; background: transparent; font-size: 10px;");

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(sizeLabel);

    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(textRegion);

    if (isSelf) {
        bubbleContent->setStyleSheet("QWidget { background-color: #20202E; border-radius: 12px; }");
        nameLabel->setStyleSheet("color: white;");
        sizeLabel->setStyleSheet("color: #AAAAAA;");
        mainLayout->addStretch();
        mainLayout->addWidget(bubbleContent);
    }
    else {
        bubbleContent->setStyleSheet("QWidget { background-color: #FFFFFF; border-radius: 12px; }");
        nameLabel->setStyleSheet("color: black;");
        sizeLabel->setStyleSheet("color: #666666;");
        mainLayout->addWidget(bubbleContent);
        mainLayout->addStretch();
    }
    return widget;
}

void ChatWindow::addMessage(const QString& msg, bool isSelf)
{
    QWidget* bubbleWidget = createBubbleWidget(msg, isSelf);
    QListWidgetItem* item = new QListWidgetItem(msgList);
    item->setSizeHint(bubbleWidget->sizeHint());
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    msgList->setItemWidget(item, bubbleWidget);
    msgList->scrollToBottom();
}

void ChatWindow::addFileMessage(const QString& fileName, const QString& fileSize, bool isSelf)
{
    QWidget* bubbleWidget = createFileBubbleWidget(fileName, fileSize, isSelf);
    QListWidgetItem* item = new QListWidgetItem(msgList);
    QSize size = bubbleWidget->sizeHint();
    item->setSizeHint(QSize(size.width(), size.height() + 10));
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    msgList->setItemWidget(item, bubbleWidget);
    msgList->scrollToBottom();
}

bool ChatWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == msgEdit && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            if (keyEvent->modifiers() & Qt::ShiftModifier)
            {
                return false;
            }
            else
            {
                onReturnPressed();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ChatWindow::onReturnPressed()
{
    QString text = msgEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // 这里不再直接画气泡了。消息要先发给服务端、存进本地，
    // 再由上层回调 appendMessage 画出来 —— 自己发的和收到的走同一条路径，
    // 不然发送失败时界面上会留一个假的"已发送"气泡
    emit sendTextMsg(text);

    msgEdit->clear();
    QTextCursor cursor = msgEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    msgEdit->setTextCursor(cursor);
    msgEdit->setFocus();
}

void ChatWindow::onFileBtnClicked()
{
    emit sendFileClicked();
}

void ChatWindow::onMoodBtnClicked()
{
    emit sendMoodClicked();
}

void ChatWindow::setChatInfo(const QString& name, const QString& status,
    const QPixmap& avatar)
{
    lbName->setText(name);
    lbStatus->setText(status);
    setAvatar(avatar);
}

void ChatWindow::setHistory(const QList<ChatMessage>& list)
{
    msgList->clear();

    for (const ChatMessage& msg : list)
        addMessage(msg.content, msg.isSelf);

    msgList->scrollToBottom();
}

void ChatWindow::appendMessage(const ChatMessage& msg)
{
    addMessage(msg.content, msg.isSelf);
}

void ChatWindow::setAvatar(const QPixmap& avatar)
{
    lbAvatar->setPixmap(avatar);
}