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

    btnNotify = new NotifyButton(leftContainer);
    btnNotify->setObjectName("BtnNotify");
    btnNotify->setFixedSize(56, 56);
    btnNotify->setIcon(QIcon(":/Resource/icon/notifications.png"));
    btnNotify->setIconSize(QSize(26, 26));
    btnNotify->setStyleSheet(
        "#BtnNotify { background-color: #EFEBFA; border: none; border-radius: 28px; }"
        "#BtnNotify:hover { background-color: #E4DCF7; }"
    );
    btnNotify->raise();      // 确保盖在列表上面

    // leftContainer 的高度会跟着窗口缩放变，装个事件过滤器盯着它的 resize，
    // 每次变了就把铃铛重新挪回右下角
    leftContainer->installEventFilter(this);


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
    connect(btnNotify, &QPushButton::clicked, this, &ScatWindow::sendNotifyClicked);
    connect(friendList, &FriendList::sendAddFriendClicked, this, &ScatWindow::sendAddFriendClicked);
    connect(friendList, &FriendList::sendFriendSelected, this, &ScatWindow::onFriendSelected);
    connect(friendList, &FriendList::sendFriendUnselected, this, &ScatWindow::onFriendUnselected);
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

    // 从缓存里取这个好友的头像和在线状态，不用再问服务端
    QJsonObject info = friendInfos.value(friendId);
    QString status = info["online"].toBool() ? "在线" : "离线";

    chatWindow->setChatInfo(friendName, status, avatarPath(info["avatar"].toString()));

    // 向上层要这个人的聊天记录
    emit requestHistory(friendId);
}

void ScatWindow::onSettingsClicked()
{
    emit sendSettingsClicked();
}

void ScatWindow::onChatTextMsgSent(const QString& msg)
{
    // 没选中任何人就不发（虽然这时候聊天界面根本没显示，保险起见）
    if (currentFriendId.isEmpty())
        return;

    emit sendTextMessage(currentFriendId, msg);
}
void ScatWindow::setUserInfo(const QString& nickname, const QString& avatar)
{
    lbMyName->setText(nickname);
    lbMyAvatar->setPixmap(QPixmap(avatarPath(avatar)));
}

QString ScatWindow::avatarPath(const QString& avatar)
{
    if (avatar.isEmpty())
        return ":/Resource/icon/head.png";

    return ":/Resource/icon/" + avatar;
}

void ScatWindow::setFriendList(const QJsonArray& friends)
{
    friendInfos.clear();
    friendList->clearFriends();

    for (const QJsonValue& value : friends) {
        QJsonObject obj = value.toObject();

        QString username = obj["username"].toString();
        QString nickname = obj["nickname"].toString();
        QString avatar = obj["avatar"].toString();

        friendInfos.insert(username, obj);

        // 列表上显示 nickname（可以改），内部标识用 username（固定不变）
        friendList->addFriendItem(username, avatarPath(avatar),nickname, obj["lastMsg"].toString());
    }

    qDebug() << "friend list loaded:" << friends.size();
}

void ScatWindow::onFriendUnselected()
{
    currentFriendId.clear();
    rightStackedWidget->setCurrentIndex(0);      // 回到默认背景页
}

void ScatWindow::updateFriendStatus(const QString& username, bool online)
{
    if (!friendInfos.contains(username))
        return;

    QJsonObject info = friendInfos.value(username);
    info["online"] = online;
    friendInfos.insert(username, info);

    // 如果正在跟这个人聊天，顺手把聊天窗顶上的状态也改掉
    if (currentFriendId == username) {
        chatWindow->setChatInfo(info["nickname"].toString(),
            online ? "在线" : "离线",
            avatarPath(info["avatar"].toString()));
    }
}

void ScatWindow::loadHistory(const QList<ChatMessage>& list)
{
    chatWindow->setHistory(list);
}

void ScatWindow::addChatMessage(const ChatMessage& msg)
{
    QString peer = msg.peer();

    // 左边列表的"最后一条消息"跟着更新
    friendList->updateLastMessage(peer, msg.content);

    // 正在看这个人的对话才画出来；在跟别人聊天就只更新列表，
    // 消息已经存进 ChatStorage 了，切回去的时候会重新读出来
    if (peer == currentFriendId)
        chatWindow->appendMessage(msg);
}

void ScatWindow::updateNotifyPos()
{
    if (!btnNotify || !leftContainer)
        return;

    const int rightMargin = 20;
    const int bottomMargin = 24;

    btnNotify->move(leftContainer->width() - btnNotify->width() - rightMargin,
        leftContainer->height() - btnNotify->height() - bottomMargin);
}

bool ScatWindow::eventFilter(QObject* watched, QEvent* event)
{
    // 左侧栏尺寸变了，铃铛跟着挪
    if (watched == leftContainer && event->type() == QEvent::Resize)
        updateNotifyPos();

    return NoFrame::eventFilter(watched, event);
}

void ScatWindow::setRequestCount(int count)
{
    if (btnNotify)
        btnNotify->setCount(count);
}