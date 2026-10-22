#include "../include/ScatWindow.h"
#include "../../Other/include/AvatarUtils.h"
#include "../../Other/include/Theme.h"
#include "../../Other/include/Motion.h"
#include <QPixmap>
#include <QSettings>
#include <QMouseEvent>

// 左上角自己的头像。比好友列表里的（45）小一号，主次分明
static const int kMyAvatarSize = 36;
static const int kFriendAvatarSize = 40;   // 好友列表
static const int kChatAvatarSize = 32;     // 聊天顶栏

// 好友栏占窗口宽度的比例，以及上下限。
// 默认窗口 920 宽时正好 220；窗口拉大跟着变宽，但不超过 320，
// 缩小时不窄于 220，免得名字被挤得只剩几个字
static const double kLeftRatio = 0.24;
static const int kLeftMinWidth = 220;
static const int kLeftMaxWidth = 320;

static const int kSplitHandleWidth = 7;     // 分隔线左右能拖的范围，线本身只有 1 像素
static const char* kLeftRatioKey = "layout/leftRatio";

ScatWindow::ScatWindow(QWidget* parent): NoFrame(parent), divider(nullptr), splitHandle(nullptr), leftRatio(kLeftRatio), draggingSplit(false)
{
    // 1. 配置 NoFrame 的基础属性
    NoFrame::Frameconfig config;
    config.background = QColor(255, 255, 255);
    config.titlebarcolor = QColor(255, 255, 255); // 白色标题栏，与内容区融合
    config.borderRadius = 10;

    // 修复 1 与 3：调整默认大小适配笔记本屏幕，并关闭最大化按钮
    config.defaultsize = QSize(920, 650);
    config.titlebarheight = 36;                   // 增加一点高度，让左上角Logo更协调

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
    centralWidget->setStyleSheet(Theme::css("#ScatCentralWidget { background-color: #FFFFFF; }"));

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0); // 左右紧密贴合

    // ================= 左侧容器 =================
    leftContainer = new QWidget(centralWidget);
    leftContainer->setFixedWidth(kLeftMinWidth); // 稍微缩窄一点侧边栏，给聊天区留出更多空间
    leftContainer->setAttribute(Qt::WA_StyledBackground, true);
    leftContainer->setStyleSheet(Theme::css("#LeftContainer { background-color: #FFFFFF; }"));

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
    btnNotify->setIcon(Theme::icon(":/Resource/icon/notifications.png"));
    btnNotify->setIconSize(QSize(26, 26));
    Motion::addRipple(btnNotify, 28);
    btnNotify->setStyleSheet(Theme::css(
        "#BtnNotify { background-color: #EFEBFA; border: none; border-radius: 28px; }"
        "#BtnNotify:hover { background-color: #E4DCF7; }"
    ));
    btnNotify->raise();      // 确保盖在列表上面

    // leftContainer 的高度会跟着窗口缩放变，装个事件过滤器盯着它的 resize，
    // 每次变了就把铃铛重新挪回右下角
    leftContainer->installEventFilter(this);


    // ================= 右侧容器 =================
        // ================= 右侧容器 =================
    rightStackedWidget = new QStackedWidget(centralWidget);

    // 页面 0: 没选中好友时的空白页。原来这里铺了一张背景图，
    // 现在留纯白，跟左侧栏之间靠那条 1px 的分割线区分
    defaultPage = new QWidget();
    defaultPage->setStyleSheet(Theme::css("background-color: #FFFFFF;"));

    // 页面 1: 聊天窗口
    chatWindow = new ChatWindow();

    // 页面 2: 设置页
    settingsPage = new SettingsPage();

    rightStackedWidget->addWidget(defaultPage);   // Index 0
    rightStackedWidget->addWidget(chatWindow);    // Index 1
    rightStackedWidget->addWidget(settingsPage);  // Index 2

    // 默认显示空白页
    rightStackedWidget->setCurrentIndex(0);

    // ================= 组合并应用到 NoFrame =================
    divider = new QWidget(centralWidget);
    divider->setFixedWidth(1);
    divider->setAttribute(Qt::WA_StyledBackground, true);
    setDividerActive(false);

    mainLayout->addWidget(leftContainer);
    mainLayout->addWidget(divider);
    mainLayout->addWidget(rightStackedWidget, 1);
    // 核心步骤：将组装好的 centralWidget 交给父类 NoFrame 渲染
    this->setMainWindow(centralWidget);
    centralWidget->installEventFilter(this);

    // 盖在分隔线上的透明拖动区域。不进布局，自己摆位置，永远在最上面
    splitHandle = new QWidget(centralWidget);
    splitHandle->setCursor(Qt::SplitHCursor);
    splitHandle->setToolTip("拖动调整宽度，双击恢复默认");
    splitHandle->installEventFilter(this);
    splitHandle->raise();

    // 上次拖出来的比例。存的值不正常就用默认的
    double saved = QSettings().value(kLeftRatioKey, kLeftRatio).toDouble();
    if (saved > 0.1 && saved < 0.6)
        leftRatio = saved;
}

void ScatWindow::initProfileSection()
{
    profileWidget = new QWidget(leftContainer);
    profileWidget->setFixedHeight(56);
    profileWidget->setStyleSheet("border: none;"); // 清除继承下来的右侧边框线影响

    QHBoxLayout* profileLayout = new QHBoxLayout(profileWidget);
    profileLayout->setContentsMargins(14, 6, 8, 6);
    profileLayout->setSpacing(10);

    // 1. 个人头像
    lbMyAvatar = new QLabel(profileWidget);
    lbMyAvatar->setFixedSize(kMyAvatarSize, kMyAvatarSize);
    lbMyAvatar->setScaledContents(true);
    lbMyAvatar->setPixmap(QPixmap(":/Resource/icon/head.png")); // 记得添加实际资源
    lbMyAvatar->setStyleSheet(Theme::css(QString("border-radius: %1px; border: 1px solid #E0E0E0;").arg(kMyAvatarSize / 2)));

    // 2. 个人名字
    lbMyName = new QLabel("Name", profileWidget);
    lbMyName->setStyleSheet(Theme::css("font-size: 15px; color: #1A1A1A; border: none;"));

    // 3. 设置按钮
    btnSettings = new QPushButton(profileWidget);
    btnSettings->setFixedSize(22, 22);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setIcon(Theme::icon(":/Resource/icon/settings.png", QColor("#9C98AA")));
    btnSettings->setIconSize(QSize(18, 18));
    Motion::addRipple(btnSettings, 11);
    btnSettings->setStyleSheet(Theme::css(
        "QPushButton { border: none; background: transparent; }"
        "QPushButton:hover { background-color: #F0F0F0; border-radius: 11px; }"
    ));

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
    connect(chatWindow, &ChatWindow::fileCancelClicked, this, &ScatWindow::fileCancelClicked);
    connect(chatWindow, &ChatWindow::fileRetryClicked, this, &ScatWindow::fileRetryClicked);
    connect(chatWindow, &ChatWindow::fileOpenClicked, this, &ScatWindow::fileOpenClicked);
    connect(chatWindow, &ChatWindow::fileDownloadClicked, this, &ScatWindow::fileDownloadClicked);
    connect(settingsPage, &SettingsPage::sendChangeAvatar, this, &ScatWindow::sendChangeAvatar);
    connect(settingsPage, &SettingsPage::sendSaveNickname, this, &ScatWindow::sendSaveNickname);
    connect(settingsPage, &SettingsPage::sendChangeStorage, this, &ScatWindow::sendChangeStorage);
    connect(settingsPage, &SettingsPage::sendRestartApp, this, &ScatWindow::sendRestartApp);

    // 通过 Lambda 捕获 currentFriendId 转发给外部
    connect(chatWindow, &ChatWindow::sendImage, this, [this](const QImage& image) {
        emit sendImage(currentFriendId, image);
        });
    // 通过 Lambda 捕获 currentFriendId 转发给外部
    connect(chatWindow, &ChatWindow::sendFileClicked, this, [this]() {
        emit sendFileClicked(currentFriendId);
        });
}

void ScatWindow::onFriendSelected(const QString& friendId, const QString& friendName)
{
    currentFriendId = friendId;

    unreadCounts.remove(friendId);
    friendList->setUnread(friendId, 0);

    if (rightStackedWidget->currentIndex() != 1)
        rightStackedWidget->setCurrentIndex(1);

    QJsonObject info = friendInfos.value(friendId);
    QString status = info["online"].toBool() ? "在线" : "离线";

    chatWindow->setChatInfo(friendName, status,
        AvatarUtils::load(info["avatar"].toString(), kChatAvatarSize));

    emit requestHistory(friendId);
}

void ScatWindow::onSettingsClicked()
{
    // 切到设置页，顺手取消好友列表的选中 ——
    // 不然右边显示设置、左边还高亮着某个好友，看着别扭
    currentFriendId.clear();
    friendList->clearSelection();
    rightStackedWidget->setCurrentIndex(2);

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
    lbMyAvatar->setPixmap(AvatarUtils::load(avatar, kMyAvatarSize));
}


void ScatWindow::setFriendList(const QJsonArray& friends)
{
    friendInfos.clear();
    friendList->clearFriends();

    for (const QJsonValue& value : friends) {
        QJsonObject obj = value.toObject();

        QString username = obj["username"].toString();
        friendInfos.insert(username, obj);

        // 列表上显示 nickname（可以改），内部标识用 username（固定不变）
        friendList->addFriendItem(username,
            AvatarUtils::load(obj["avatar"].toString(), kFriendAvatarSize),
            obj["nickname"].toString(),
            obj["lastMsg"].toString(),
            obj["lastTime"].toVariant().toLongLong());

        // 列表是整体重建的，未读数要重新贴回去，
        // 否则好友改个昵称触发重建，红点就凭空消失了
        friendList->setUnread(username, unreadCounts.value(username));
    }
    friendList->sortByTime();
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

    if (currentFriendId == username) {
        chatWindow->setChatInfo(info["nickname"].toString(),
            online ? "在线" : "离线",
            AvatarUtils::load(info["avatar"].toString(), kChatAvatarSize));
    }
}

void ScatWindow::loadHistory(const QList<ChatMessage>& list)
{
    chatWindow->setHistory(list);
}

void ScatWindow::addChatMessage(const ChatMessage& msg)
{
    QString peer = msg.peer();

    // 左边列表的"最后一条消息"跟着更新，这个人顶到最上面。
    // 自己发的也算，跟 QQ、微信一样
    friendList->updateLastMessage(peer, msg.content, msg.time);

    // 正在看这个人的对话才画出来；在跟别人聊天就只更新列表，
    // 消息已经存进 ChatStorage 了，切回去的时候会重新读出来
    if (peer == currentFriendId) {
        chatWindow->appendMessage(msg);
        return;
    }

    // 不是当前正在看的会话，而且是别人发来的（自己发的不算未读）
    if (!msg.isSelf) {
        int count = unreadCounts.value(peer) + 1;
        unreadCounts.insert(peer, count);
        friendList->setUnread(peer, count);
    }
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
    if (watched == leftContainer && event->type() == QEvent::Resize) {
        updateNotifyPos();
        updateSplitHandle();
    }

    // 窗口尺寸变了，好友栏按比例跟着变
    if (watched == centralWidget && event->type() == QEvent::Resize) {
        updateLeftWidth();
        updateSplitHandle();
    }

    // 拖动分隔线
    if (watched == splitHandle && handleSplitEvent(event))
        return true;

    return NoFrame::eventFilter(watched, event);
}

void ScatWindow::setRequestCount(int count)
{
    if (btnNotify)
        btnNotify->setCount(count);
}

void ScatWindow::setSettingsInfo(const QString& username, const QString& nickname,
    const QString& avatar)
{
    if (settingsPage)
        settingsPage->setUserInfo(username, nickname, avatar);
}

void ScatWindow::setStoragePath(const QString& path)
{
    if (settingsPage)
        settingsPage->setStoragePath(path);
}

void ScatWindow::onNicknameSaved(bool ok, const QString& reason)
{
    if (settingsPage)
        settingsPage->onNicknameSaved(ok, reason);
}

void ScatWindow::updateMyNickname(const QString& nickname)
{
    // 左上角个人信息栏
    lbMyName->setText(nickname);

    // 设置页里的输入框（服务端可能做了 trim，以它返回的为准）
    if (settingsPage)
        settingsPage->setNickname(nickname);
}

void ScatWindow::updateMyAvatar(const QString& avatar)
{
    lbMyAvatar->setPixmap(AvatarUtils::load(avatar, kMyAvatarSize));

    if (settingsPage)
        settingsPage->setAvatar(avatar);
}

void ScatWindow::refreshAvatar(const QString& avatar)
{
    // 刚下载完某个头像文件，把用这张图的地方都刷一遍。
    // 理论上只会有一个人用它（文件名带账号和时间戳），循环是为了保险
    for (auto it = friendInfos.constBegin(); it != friendInfos.constEnd(); ++it) {
        if (it.value()["avatar"].toString() != avatar)
            continue;

        friendList->updateAvatar(it.key(), AvatarUtils::load(avatar, kFriendAvatarSize));

        if (it.key() == currentFriendId)
            chatWindow->setAvatar(AvatarUtils::load(avatar, kChatAvatarSize));
    }
}

void ScatWindow::onAvatarUploaded(bool ok, const QString& reason)
{
    if (settingsPage)
        settingsPage->onAvatarUploaded(ok, reason);
}

void ScatWindow::selectFriend(const QString& username)
{
    friendList->selectFriend(username);
}

QString ScatWindow::nicknameOf(const QString& username) const
{
    QJsonObject info = friendInfos.value(username);
    QString nickname = info["nickname"].toString();

    return nickname.isEmpty() ? username : nickname;
}

QPixmap ScatWindow::avatarOf(const QString& username, int size) const
{
    QJsonObject info = friendInfos.value(username);
    return AvatarUtils::load(info["avatar"].toString(), size);
}

void ScatWindow::updateFileProgress(const QString& msgid, qint64 done, qint64 total)
{
    chatWindow->updateFileProgress(msgid, done, total);
}

void ScatWindow::updateFileState(const QString& msgid, int state,
    const QString& filePath)
{
    chatWindow->updateFileState(msgid, state, filePath);
}

void ScatWindow::updateLeftWidth()
{
    int width = qBound(kLeftMinWidth, qRound(centralWidget->width() * leftRatio), kLeftMaxWidth);

    if (leftContainer->width() != width)
        leftContainer->setFixedWidth(width);
}

void ScatWindow::updateSplitHandle()
{
    if (!splitHandle)
        return;

    // 分隔线紧挨着好友栏右边，拖动区域以它为中心左右各伸出几像素
    int x = leftContainer->width() - kSplitHandleWidth / 2;
    splitHandle->setGeometry(x, 0, kSplitHandleWidth, centralWidget->height());
    splitHandle->raise();
}

void ScatWindow::setDividerActive(bool active)
{
    divider->setStyleSheet(Theme::css(active ? "background-color: #C9C1D8;"
        : "background-color: #EAEAEA;"));
}

bool ScatWindow::handleSplitEvent(QEvent* event)
{
    switch (event->type()) {
    case QEvent::Enter:
        setDividerActive(true);
        return false;

    case QEvent::Leave:
        if (!draggingSplit)
            setDividerActive(false);
        return false;

    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent*>(event)->button() != Qt::LeftButton)
            return false;
        draggingSplit = true;
        return true;

    case QEvent::MouseMove: {
        if (!draggingSplit)
            return false;

        // 鼠标在哪，好友栏就拉到哪，但不超出上下限。
        // 记下的是比例，之后再缩放窗口还按这个比例走
        QMouseEvent* e = static_cast<QMouseEvent*>(event);
        int x = centralWidget->mapFromGlobal(e->globalPosition().toPoint()).x();
        int width = qBound(kLeftMinWidth, x, kLeftMaxWidth);

        leftRatio = double(width) / centralWidget->width();
        leftContainer->setFixedWidth(width);
        return true;
    }

    case QEvent::MouseButtonRelease:
        if (!draggingSplit)
            return false;
        draggingSplit = false;
        if (!splitHandle->underMouse())
            setDividerActive(false);

        // 松手才存，拖的过程中不用一直写设置
        QSettings().setValue(kLeftRatioKey, leftRatio);
        return true;

    case QEvent::MouseButtonDblClick:
        // 双击恢复默认比例
        leftRatio = kLeftRatio;
        QSettings().remove(kLeftRatioKey);
        updateLeftWidth();
        return true;

    default:
        return false;
    }
}