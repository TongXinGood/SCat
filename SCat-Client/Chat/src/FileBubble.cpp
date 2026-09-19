#include "../include/FileBubble.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>

// 气泡卡片的固定尺寸。固定死是有意的 —— 高度一变，外面
// QListWidgetItem 的 sizeHint 就得跟着重算，麻烦还容易出错
static const int kCardW = 260;
static const int kCardH = 76;

static QString formatSize(qint64 bytes)
{
    if (bytes >= 1024LL * 1024 * 1024)
        return QString::number(bytes / 1024.0 / 1024 / 1024, 'f', 1) + " GB";
    if (bytes >= 1024 * 1024)
        return QString::number(bytes / 1024.0 / 1024, 'f', 1) + " MB";
    if (bytes >= 1024)
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";

    return QString::number(bytes) + " B";
}

FileBubble::FileBubble(const ChatMessage& msg, QWidget* parent)
    : QWidget(parent)
    , id(msg.msgid)
    , peer(msg.peer())
    , path(msg.filePath)
    , name(msg.fileName)
    , size(msg.fileSize)
    , self(msg.isSelf)
    , state(msg.fileState)
    , card(nullptr)
    , lbIcon(nullptr)
    , lbName(nullptr)
    , lbInfo(nullptr)
    , bar(nullptr)
    , btnAction(nullptr)
{
    initUI();
    refresh();
}

void FileBubble::initUI()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: transparent;");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(10, 10, 10, 10);

    card = new QWidget(this);
    card->setObjectName("FileCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setFixedSize(kCardW, kCardH);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // ---- 上半部分：图标 + 文字 + 按钮 ----
    QWidget* top = new QWidget(card);
    top->setObjectName("Plain");

    QHBoxLayout* topLayout = new QHBoxLayout(top);
    topLayout->setContentsMargins(14, 10, 12, 8);
    topLayout->setSpacing(10);

    lbIcon = new QLabel(top);
    lbIcon->setObjectName("FileIcon");
    lbIcon->setFixedSize(36, 36);
    lbIcon->setPixmap(QPixmap(":/Resource/icon/folder.png")
        .scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lbIcon->setAlignment(Qt::AlignCenter);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(3);

    lbName = new QLabel(top);
    lbName->setObjectName("FileName");
    // 文件名可能很长，中间打省略号，完整的放 tooltip
    lbName->setText(QFontMetrics(lbName->font())
        .elidedText(name, Qt::ElideMiddle, 150));
    lbName->setToolTip(name);

    lbInfo = new QLabel(top);
    lbInfo->setObjectName("FileInfo");

    textLayout->addWidget(lbName);
    textLayout->addWidget(lbInfo);

    btnAction = new QPushButton(top);
    btnAction->setObjectName("FileAction");
    btnAction->setFixedHeight(24);
    btnAction->setCursor(Qt::PointingHandCursor);

    topLayout->addWidget(lbIcon);
    topLayout->addLayout(textLayout, 1);
    topLayout->addWidget(btnAction, 0, Qt::AlignTop);

    // ---- 下半部分：4px 的细进度条 ----
    // 做成细条是为了卡片高度不用变：传完了把它藏起来，
    // 也就损失 4 个像素的空白，视觉上根本看不出来
    bar = new QProgressBar(card);
    bar->setObjectName("FileBar");
    bar->setFixedHeight(4);
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(false);

    cardLayout->addWidget(top, 1);
    cardLayout->addWidget(bar);

    if (self) {
        outer->addStretch();
        outer->addWidget(card);
    }
    else {
        outer->addWidget(card);
        outer->addStretch();
    }

    connect(btnAction, &QPushButton::clicked, this, &FileBubble::onActionClicked);

    card->setStyleSheet(R"(
        QWidget#FileCard {
            background-color: #FFFFFF;
            border: 1px solid #ECE9F2;
            border-radius: 12px;
        }
        QWidget#Plain  { background: transparent; border: none; }
        QLabel#FileIcon { background: transparent; border: none; }
        QLabel#FileName {
            font-size: 13px; font-weight: bold; color: #1A1A1A;
            background: transparent; border: none;
        }
        QLabel#FileInfo {
            font-size: 11px; color: #9E9E9E;
            background: transparent; border: none;
        }

        QPushButton#FileAction {
            background-color: #F4F1FA;
            color: #3A3A4A;
            border: none;
            border-radius: 6px;
            font-size: 11px;
            padding-left: 8px;
            padding-right: 8px;
        }
        QPushButton#FileAction:hover  { background-color: #E7E1F5; }
        QPushButton#FileAction:pressed{ background-color: #DCD4F0; }

        QProgressBar#FileBar {
            background-color: #F0EDF6;
            border: none;
            border-bottom-left-radius: 12px;
            border-bottom-right-radius: 12px;
        }
        QProgressBar#FileBar::chunk {
            background-color: #20202E;
            border-bottom-left-radius: 12px;
        }
    )");
}

void FileBubble::setProgress(qint64 done, qint64 total)
{
    int percent = total > 0 ? int(done * 100 / total) : 0;

    bar->setValue(percent);
    progressText = QString::number(percent) + "%";

    refresh();
}

void FileBubble::setState(int newState)
{
    state = newState;

    // 不在传输中了，进度文字就别留着了
    if (state != FILE_STATE_SENDING && state != FILE_STATE_DOWNLOADING)
        progressText.clear();

    refresh();
}

void FileBubble::refresh()
{
    QString status;
    QString action;
    bool showBar = false;

    switch (state) {
    case FILE_STATE_SENDING:
        status = progressText.isEmpty() ? "发送中" : "发送中 " + progressText;
        action = "取消";
        showBar = true;
        break;

    case FILE_STATE_SENT:
        status = "已发送";
        action = "打开";
        break;

    case FILE_STATE_READY:
        // 对方发来的，文件在服务端躺着，下不下是用户说了算
        status = "等待下载";
        action = "下载";
        break;

    case FILE_STATE_DOWNLOADING:
        status = progressText.isEmpty() ? "下载中" : "下载中 " + progressText;
        action = "取消";
        showBar = true;
        break;

    case FILE_STATE_DONE:
        status = "已下载";
        action = "打开";
        break;

    case FILE_STATE_CANCELED:
        status = "已取消";
        // 自己发的是重传，别人发来的是重新下
        action = self ? "重试" : "下载";
        break;

    case FILE_STATE_FAILED:
    default:
        status = self ? "发送失败" : "下载失败";
        action = self ? "重试" : "下载";
        break;
    }

    lbInfo->setText(formatSize(size) + " · " + status);

    btnAction->setText(action);
    btnAction->setVisible(!action.isEmpty());

    bar->setVisible(showBar);
}

void FileBubble::onActionClicked()
{
    switch (state) {
    case FILE_STATE_SENDING:
    case FILE_STATE_DOWNLOADING:
        emit cancelClicked(id);
        break;

    case FILE_STATE_SENT:
    case FILE_STATE_DONE:
        emit openClicked(id, path);
        break;

    case FILE_STATE_READY:
        emit downloadClicked(id);
        break;

    default:
        // 失败 / 取消。自己发的是重传源文件，别人发来的是重新下
        if (self)
            emit retryClicked(id, peer, path);
        else
            emit downloadClicked(id);
        break;
    }
}