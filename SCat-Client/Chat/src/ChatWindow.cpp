#include "../include/ChatWindow.h"
#include "../../Other/include/ImageUtils.h"
#include <QDebug>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QUrl>

static const int kImageMaxW = 240;
static const int kImageMaxH = 320;

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
    msgEdit = new ChatInputEdit(inputContainer);
    msgEdit->setPlaceholderText("Enter your message");
    msgEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    msgEdit->setStyleSheet(
        "QTextEdit {"
        "   border: none;"
        "   background: transparent;"
        "   font-size: 14px;"
        "   color: #333;"
        "   padding-top: 6px;"
        "}"
    );
    msgEdit->setFixedHeight(40);
    connect(msgEdit, &ChatInputEdit::sendPressed, this, &ChatWindow::onReturnPressed);
    connect(msgEdit, &ChatInputEdit::imagePasted, this, &ChatWindow::sendImage);

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

QWidget* ChatWindow::createImageBubbleWidget(const ChatMessage& msg)
{
    QWidget* widget = new QWidget();
    widget->setStyleSheet("background-color: transparent;");
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel* bubble = new QLabel();
    QString path = ImageUtils::pathFor(msg.imgName);
    QPixmap pix(path);

    if (pix.isNull()) {
        // 文件没了（用户手动删的，或者换数据目录时漏拷了），
        // 退化成一个文字气泡，别留一块空白让人莫名其妙
        bubble->setText("[图片已失效]");
        bubble->setStyleSheet("QLabel { background-color: #FFFFFF; color: #999999; "
            "border-radius: 12px; padding: 12px 16px; }");
    }
    else {
        // 宽高是存库时记下来的，这里不用解码整张图就能定尺寸
        QSize show = ImageUtils::fitSize(msg.imgW, msg.imgH, kImageMaxW, kImageMaxH);

        bubble->setFixedSize(show);
        bubble->setPixmap(pix.scaled(show, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        bubble->setCursor(Qt::PointingHandCursor);
        bubble->setStyleSheet("QLabel { background: transparent; border: none; }");

        // QLabel 没有 clicked 信号。把路径挂在属性上，
        // 统一交给 eventFilter 处理，不用为了点一下再造个类
        bubble->setProperty("imagePath", path);
        bubble->installEventFilter(this);
    }

    if (msg.isSelf) {
        layout->addStretch();
        layout->addWidget(bubble);
    }
    else {
        layout->addWidget(bubble);
        layout->addStretch();
    }

    return widget;
}

void ChatWindow::addBubble(const ChatMessage& msg)
{
    QWidget* bubbleWidget = nullptr;

    if (msg.kind == KIND_IMAGE)
        bubbleWidget = createImageBubbleWidget(msg);
    else if (msg.kind == KIND_FILE)
        bubbleWidget = createFileBubbleWidget(msg);
    else
        bubbleWidget = createBubbleWidget(msg.content, msg.isSelf);

    QListWidgetItem* item = new QListWidgetItem(msgList);
    item->setSizeHint(bubbleWidget->sizeHint());
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    msgList->setItemWidget(item, bubbleWidget);
    msgList->scrollToBottom();
}

QWidget* ChatWindow::createFileBubbleWidget(const ChatMessage& msg)
{
    FileBubble* bubble = new FileBubble(msg);

    // 记进表里，进度信号回来时才找得到它。
    // 这张表在 setHistory 里会被清空重建 —— 切会话时 QListWidget
    // 会把所有 item widget 销毁，表里留着就全是野指针了
    fileBubbles.insert(msg.msgid, bubble);

    connect(bubble, &FileBubble::cancelClicked, this, &ChatWindow::fileCancelClicked);
    connect(bubble, &FileBubble::retryClicked, this, &ChatWindow::fileRetryClicked);
    connect(bubble, &FileBubble::openClicked, this, &ChatWindow::fileOpenClicked);
    connect(bubble, &FileBubble::downloadClicked, this, &ChatWindow::fileDownloadClicked);

    return bubble;
}


bool ChatWindow::eventFilter(QObject* watched, QEvent* event)
{
    /*if (watched == msgEdit && event->type() == QEvent::KeyPress)
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
    }*/

    if (event->type() == QEvent::MouseButtonRelease)
    {
        QString path = watched->property("imagePath").toString();
        if (!path.isEmpty())
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            return true;
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
    fileBubbles.clear();

    msgList->clear();

    for (const ChatMessage& msg : list)
        addBubble(msg);

    msgList->scrollToBottom();
}

void ChatWindow::appendMessage(const ChatMessage& msg)
{
    addBubble(msg);
}

void ChatWindow::setAvatar(const QPixmap& avatar)
{
    lbAvatar->setPixmap(avatar);
}

void ChatWindow::updateFileProgress(const QString& msgid, qint64 done, qint64 total)
{
    FileBubble* bubble = fileBubbles.value(msgid, nullptr);

    // 找不到说明这条消息不在当前会话里（用户切到别人那儿去了）。
    // 传输本身在 FileTransfer 里照常跑，切回来时气泡会按最新状态重建
    if (bubble)
        bubble->setProgress(done, total);
}

void ChatWindow::updateFileState(const QString& msgid, int state,
    const QString& filePath)
{
    FileBubble* bubble = fileBubbles.value(msgid, nullptr);
    if (!bubble)
        return;

    if (!filePath.isEmpty())
        bubble->setFilePath(filePath);

    bubble->setState(state);
}