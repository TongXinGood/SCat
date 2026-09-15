#include "../include/AddFriendWindow.h"
#include "../../Other/include/AvatarUtils.h"
#include <QDebug>

AddFriendWindow::AddFriendWindow(QWidget* parent) : NoFrame(parent)
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
    initConnect();

    clearResult();
}

AddFriendWindow::~AddFriendWindow()
{
}

void AddFriendWindow::initUI()
{
    content = new QWidget(this);
    content->setObjectName("AddFriendContent");

    mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(24, 8, 24, 24);
    mainLayout->setSpacing(18);

    // ============ 搜索栏（胶囊形，淡紫底） ============
    searchContainer = new QWidget(content);
    searchContainer->setObjectName("SearchBar");
    searchContainer->setFixedHeight(46);
    searchContainer->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(18, 0, 18, 0);
    searchLayout->setSpacing(10);

    lbSearchIcon = new QLabel(searchContainer);
    lbSearchIcon->setObjectName("SearchIcon");
    lbSearchIcon->setFixedSize(18, 18);
    lbSearchIcon->setScaledContents(true);
    lbSearchIcon->setPixmap(QPixmap(":/Resource/icon/search.png"));

    editSearch = new QLineEdit(searchContainer);
    editSearch->setObjectName("SearchEdit");
    editSearch->setPlaceholderText("输入账号后按回车搜索");
    editSearch->setMaxLength(15);          // username 最长 15 位

    searchLayout->addWidget(lbSearchIcon);
    searchLayout->addWidget(editSearch);

    // ============ 提示文字（空态 / 搜索中 / 未找到） ============
    lbHint = new QLabel(content);
    lbHint->setObjectName("Hint");
    lbHint->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    lbHint->setWordWrap(true);

    // ============ 搜索结果行 ============
    resultWidget = new QWidget(content);
    resultWidget->setObjectName("ResultRow");
    resultWidget->setFixedHeight(72);
    resultWidget->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* resultLayout = new QHBoxLayout(resultWidget);
    resultLayout->setContentsMargins(12, 10, 12, 10);
    resultLayout->setSpacing(12);

    lbAvatar = new QLabel(resultWidget);
    lbAvatar->setObjectName("ResultAvatar");
    lbAvatar->setFixedSize(48, 48);
    // 头像由 AvatarUtils 裁好成 48x48 的圆形再塞进来，所以：
    //   1. 不能开 setScaledContents，会把图拉变形
    //   2. 背景必须透明，圆形图四角是透明的，底下有色块会露出方角

    QVBoxLayout* nameLayout = new QVBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(3);

    lbNickname = new QLabel(resultWidget);
    lbNickname->setObjectName("Nickname");

    lbUsername = new QLabel(resultWidget);
    lbUsername->setObjectName("Username");

    nameLayout->addStretch();
    nameLayout->addWidget(lbNickname);
    nameLayout->addWidget(lbUsername);
    nameLayout->addStretch();

    btnAdd = new QPushButton(resultWidget);
    btnAdd->setObjectName("BtnAdd");
    btnAdd->setFixedSize(72, 32);
    btnAdd->setCursor(Qt::PointingHandCursor);

    resultLayout->addWidget(lbAvatar);
    resultLayout->addLayout(nameLayout);
    resultLayout->addStretch();
    resultLayout->addWidget(btnAdd);

    // ============ 组装 ============
    mainLayout->addWidget(searchContainer);
    mainLayout->addWidget(resultWidget);
    mainLayout->addWidget(lbHint);
    mainLayout->addStretch();

    this->setMainWindow(content);

    QString qss = R"(
        #AddFriendContent { background-color: #FFFFFF; }

        #SearchBar {
            background-color: #F4F1FA;
            border-radius: 23px;
        }
        #SearchIcon { background: transparent; border: none; }
        #SearchEdit {
            border: none;
            background: transparent;
            font-size: 14px;
            color: #333333;
        }

        #Hint {
            font-size: 13px;
            color: #999999;
            padding-top: 20px;
        }

        #ResultRow {
            background-color: #FFFFFF;
            border-radius: 10px;
            border: 1px solid #EEEEEE;
        }
        #ResultAvatar {
            background: transparent;
            border: none;
        }
        #Nickname {
            font-size: 15px;
            font-weight: bold;
            color: #1A1A1A;
            background: transparent;
            border: none;
        }
        #Username {
            font-size: 12px;
            color: #999999;
            background: transparent;
            border: none;
        }

        #BtnAdd {
            background-color: #20202E;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            font-size: 13px;
        }
        #BtnAdd:hover { background-color: #35354A; }
        #BtnAdd:disabled {
            background-color: #EDEDED;
            color: #AAAAAA;
        }
    )";

    content->setStyleSheet(qss);
}

void AddFriendWindow::initConnect()
{
    connect(editSearch, &QLineEdit::returnPressed, this, &AddFriendWindow::onSearchReturn);
    connect(btnAdd, &QPushButton::clicked, this, &AddFriendWindow::onAddClicked);
}

void AddFriendWindow::onSearchReturn()
{
    QString name = editSearch->text().trimmed();
    if (name.isEmpty())
        return;

    showSearching();
    emit sendSearchUser(name);
}

void AddFriendWindow::onAddClicked()
{
    if (resultUser.isEmpty())
        return;

    // 先把按钮锁上，避免连点发出多条申请。
    // 服务端回来之后 onAddSent 再决定最终文字
    btnAdd->setEnabled(false);
    btnAdd->setText("发送中");

    emit sendAddFriend(resultUser);
}

// ---------- 给逻辑层调的接口 ----------

void AddFriendWindow::setHint(const QString& text)
{
    resultWidget->hide();
    lbHint->setText(text);
    lbHint->show();
}

void AddFriendWindow::clearResult()
{
    resultUser.clear();
    editSearch->clear();
    setHint("输入对方的账号，按回车查找");
}

void AddFriendWindow::showSearching()
{
    setHint("搜索中…");
}

void AddFriendWindow::showNotFound()
{
    resultUser.clear();
    setHint("没有找到这个账号");
}

void AddFriendWindow::showResult(const QString& username, const QString& nickname,
    const QString& avatar, int relation)
{
    resultUser = username;

    lbAvatar->setPixmap(AvatarUtils::load(avatar, 48));
    lbNickname->setText(nickname);
    lbUsername->setText(username);

    // 按钮状态只有三种，"已申请"不在这里出现 ——
    // 那是点过之后的反馈，重新搜一次就又能申请了
    switch (relation) {
    case RELATION_FRIEND:
        btnAdd->setText("已是好友");
        btnAdd->setEnabled(false);
        break;
    case RELATION_SELF:
        btnAdd->setText("是你自己");
        btnAdd->setEnabled(false);
        break;
    default:
        btnAdd->setText("添加");
        btnAdd->setEnabled(true);
        break;
    }

    lbHint->hide();
    resultWidget->show();
}

void AddFriendWindow::onAddSent(bool ok, const QString& reason)
{
    if (ok) {
        // 变灰、显示"已申请"。关掉窗口重新搜同一个人，
        // 按钮会重新变回可点的"添加"
        btnAdd->setText("已申请");
        btnAdd->setEnabled(false);
    }
    else {
        // 发失败了，让用户能重试
        btnAdd->setText("添加");
        btnAdd->setEnabled(true);
        qDebug() << "add friend failed:" << reason;
    }
}
