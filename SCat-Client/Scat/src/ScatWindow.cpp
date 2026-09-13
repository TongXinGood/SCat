#include "../include/ScatWindow.h"
#include <QPixmap>

ScatWindow::ScatWindow(QWidget* parent) : NoFrame(parent)
{
    // 1. 配置 NoFrame 的基础属性
    NoFrame::Frameconfig config;
    config.background = QColor(255, 255, 255);
    config.titlebarcolor = QColor(255, 255, 255); // 白色标题栏，与内容区融合
    config.borderRadius = 10;

    // 修复 1 与 3：调整默认大小适配笔记本屏幕，并关闭最大化按钮
    config.defaultsize = QSize(850, 600);
    config.titlebarheight = 40;                   // 增加一点高度，让左上角Logo更协调

    // 修复 2：开启左上角Logo与文本显示
    config.showlogo = true;
    config.showtext = true;
    config.showmin = true;
    config.showmax = false;                       // 隐藏最大化按钮
    config.showclose = true;
    config.btnsize = QSize(20, 20);
    this->setFrameconfig(config);

    // 设置标题名称，对应配置中的 showtext
    this->setWindowTitle("SCat");

    // 强制声明最小尺寸限制，确保高度能够被用户自由缩小
    this->setMinimumSize(700, 500);

    initUI();
    initConnect();
}

ScatWindow::~ScatWindow()
{
}

void ScatWindow::initUI()
{
    // 1. 实例化主内容容器
    centralWidget = new QWidget(this);
    centralWidget->setObjectName("ScatCentralWidget");
    centralWidget->setStyleSheet("#ScatCentralWidget { background-color: #FFFFFF; }");

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0); // 左右紧密贴合

    // ================= 左侧容器 =================
    leftContainer = new QWidget(centralWidget);
    leftContainer->setFixedWidth(300); // 稍微缩窄一点侧边栏，给聊天区留出更多空间
    leftContainer->setAttribute(Qt::WA_StyledBackground, true);
    leftContainer->setStyleSheet("background-color: #FFFFFF; border-right: 1px solid #EAEAEA;");

    leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    initProfileSection();

    friendList = new FriendList(leftContainer);

    leftLayout->addWidget(profileWidget);
    leftLayout->addWidget(friendList);

    // ================= 右侧容器 =================
    rightStackedWidget = new QStackedWidget(centralWidget);

    // 页面 0: 默认占位背景
    defaultPage = new QWidget();
    defaultPage->setStyleSheet("background-color: #F2F0F5;"); // 与聊天窗口背景保持一致
    QHBoxLayout* defaultLayout = new QHBoxLayout(defaultPage);
    defaultLayout->setContentsMargins(0, 0, 0, 0);

    lbDefaultBg = new QLabel(defaultPage);
    lbDefaultBg->setAlignment(Qt::AlignCenter);
    lbDefaultBg->setPixmap(QPixmap(":/Resource/icon/background.png"));
    lbDefaultBg->setScaledContents(false);

    defaultLayout->addWidget(lbDefaultBg);

    // 页面 1: 聊天窗口
    chatWindow = new ChatWindow();

    rightStackedWidget->addWidget(defaultPage); // Index 0
    rightStackedWidget->addWidget(chatWindow);  // Index 1

    // 默认显示背景图
    rightStackedWidget->setCurrentIndex(0);

    // ================= 组合并应用到 NoFrame =================
    mainLayout->addWidget(leftContainer);
    mainLayout->addWidget(rightStackedWidget, 1);

    // 核心步骤：将组装好的 centralWidget 交给父类 NoFrame 渲染
    this->setMainWindow(centralWidget);
}

void ScatWindow::initProfileSection()
{
    profileWidget = new QWidget(leftContainer);
    profileWidget->setFixedHeight(80);
    profileWidget->setStyleSheet("border: none;"); // 清除继承下来的右侧边框线影响

    QHBoxLayout* profileLayout = new QHBoxLayout(profileWidget);
    profileLayout->setContentsMargins(20, 10, 20, 10);
    profileLayout->setSpacing(15);

    // 1. 个人头像
    lbMyAvatar = new QLabel(profileWidget);
    lbMyAvatar->setFixedSize(50, 50);
    lbMyAvatar->setScaledContents(true);
    lbMyAvatar->setPixmap(QPixmap(":/Resource/icon/head.png")); // 记得添加实际资源
    lbMyAvatar->setStyleSheet("border-radius: 25px; border: 1px solid #E0E0E0;");

    // 2. 个人名字
    lbMyName = new QLabel("Name", profileWidget);
    lbMyName->setStyleSheet("font-size: 18px; font-weight: bold; color: #1A1A1A; border: none;");

    // 3. 设置按钮
    btnSettings = new QPushButton(profileWidget);
    btnSettings->setFixedSize(24, 24);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setIcon(QIcon(":/Resource/icon/settings.png")); // 记得添加实际资源
    btnSettings->setIconSize(QSize(20, 20));
    btnSettings->setStyleSheet(
        "QPushButton { border: none; background: transparent; }"
        "QPushButton:hover { background-color: #F0F0F0; border-radius: 12px; }"
    );

    profileLayout->addWidget(lbMyAvatar);
    profileLayout->addWidget(lbMyName);
    profileLayout->addStretch(1);
    profileLayout->addWidget(btnSettings);
}

void ScatWindow::initConnect()
{
    // 绑定左侧信号
    connect(btnSettings, &QPushButton::clicked, this, &ScatWindow::onSettingsClicked);
    connect(friendList, &FriendList::sendAddFriendClicked, this, &ScatWindow::sendAddFriendClicked);
    connect(friendList, &FriendList::sendFriendSelected, this, &ScatWindow::onFriendSelected);

    // 绑定右侧信号
    connect(chatWindow, &ChatWindow::sendTextMsg, this, &ScatWindow::onChatTextMsgSent);

    // 通过 Lambda 捕获 currentFriendId 转发给外部
    connect(chatWindow, &ChatWindow::sendFileClicked, this, [this]() {
        emit sendFileClicked(currentFriendId);
        });
    connect(chatWindow, &ChatWindow::sendMoodClicked, this, [this]() {
        emit sendMoodClicked(currentFriendId);
        });
}

void ScatWindow::onFriendSelected(const QString& friendId, const QString& friendName)
{
    currentFriendId = friendId;

    // 切换到聊天界面
    if (rightStackedWidget->currentIndex() != 1) {
        rightStackedWidget->setCurrentIndex(1);
    }

    // 设置聊天窗口顶部的信息
    QString avatarPath = QString(":/Resource/avatar/%1.png").arg(friendId);
    chatWindow->setChatInfo(friendName, "Active now", avatarPath);
}

void ScatWindow::onSettingsClicked()
{
    emit sendSettingsClicked();
}

void ScatWindow::onChatTextMsgSent(const QString& msg)
{
    emit sendTextMessage(currentFriendId, msg);
}
void ScatWindow::setUserInfo(const QString& nickname, const QString& avatar)
{
    lbMyName->setText(nickname);
    lbMyAvatar->setPixmap(QPixmap(":/Resource/icon/" + avatar));
}
