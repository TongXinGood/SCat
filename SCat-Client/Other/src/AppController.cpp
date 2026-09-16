#include "../include/AppController.h"
#include "../include/UserSession.h"
#include "../include/AppPath.h"
#include "../include/AvatarUtils.h"
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , net(nullptr)
    , loginLogic(nullptr)
    , regLogic(nullptr)
    , loginWin(nullptr)
    , regWin(nullptr)
    , scatWin(nullptr)
    , friendMgr(nullptr)
    , chatNet(nullptr)
    , storage(nullptr)
    , addFriendWin(nullptr)
    , requestWin(nullptr)
    , settingsMgr(nullptr)
    , notify(nullptr)
    , pendingCount(0)
{
    // 网络层
    net = new NetWorkManager(this);

    // 业务层（注入网络）
    loginLogic = new Login(net, this);
    regLogic = new Register(net, this);
    friendMgr = new FriendManager(net, this);
    chatNet = new ChatNetWork(net, this);
    storage = new ChatStorage(this);
    settingsMgr = new SettingsManager(net, this);
    notify = new Notification(this);

    // 业务结果
    connect(loginLogic, &Login::loginSuccess, this, &AppController::onLoginSuccess);
    connect(loginLogic, &Login::loginFailed, this, &AppController::onLoginFailed);
    connect(regLogic, &Register::registerSuccess, this, &AppController::onRegisterSuccess);
    connect(regLogic, &Register::registerFailed, this, &AppController::onRegisterFailed);
    connect(friendMgr, &FriendManager::friendListReady, this, &AppController::onFriendListReady);
    connect(friendMgr, &FriendManager::friendListFailed, this, &AppController::onFriendListFailed);
    connect(friendMgr, &FriendManager::friendStatusChanged, this, &AppController::onFriendStatusChanged);
    connect(friendMgr, &FriendManager::pendingListReady, this, &AppController::onPendingListReady);
    connect(friendMgr, &FriendManager::newRequestArrived, this, &AppController::onNewRequestArrived);
    connect(friendMgr, &FriendManager::requestHandled, this, &AppController::onRequestHandled);
    connect(friendMgr, &FriendManager::friendListChanged, this, &AppController::onFriendListChanged);
    connect(chatNet, &ChatNetWork::messageSent, this, &AppController::onMessageSent);
    connect(chatNet, &ChatNetWork::messageReceived, this, &AppController::onMessageReceived);
    connect(chatNet, &ChatNetWork::sendFailed, this, &AppController::onChatSendFailed);
    connect(settingsMgr, &SettingsManager::nicknameSaved, this, &AppController::onNicknameSaved);
    connect(settingsMgr, &SettingsManager::avatarUploaded, this, &AppController::onAvatarUploaded);
    connect(settingsMgr, &SettingsManager::avatarDownloaded, this, &AppController::onAvatarDownloaded);
    connect(notify, &Notification::notificationClicked, this, &AppController::onNotificationClicked);
    connect(notify, &Notification::trayActivated, this, &AppController::onTrayActivated);

    connect(net, &NetWorkManager::errorOccurred, this, &AppController::onNetError);
}

AppController::~AppController()
{
    delete loginWin;
    delete regWin;
    delete scatWin;
    delete addFriendWin;
    delete requestWin;
}

void AppController::start()
{
    // 服务器地址放在 exe 旁边的 config.ini 里。
    // 换机器部署时只要改这个文件，不用重新编译
    QString cfgPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings cfg(cfgPath, QSettings::IniFormat);

    QString host = cfg.value("server/host", "127.0.0.1").toString();
    quint16 port = static_cast<quint16>(cfg.value("server/port", 8888).toUInt());

    // 文件不存在就按默认值生成一份，用户拿到手就知道能改什么
    if (!QFile::exists(cfgPath)) {
        cfg.setValue("server/host", host);
        cfg.setValue("server/port", port);
        cfg.sync();
        qDebug() << "config.ini created:" << cfgPath;
    }

    qDebug() << "connecting to" << host << ":" << port;

    net->connectToServer(host, port);
    showLoginWindow();
}

// ---------- 窗口切换 ----------

void AppController::showLoginWindow()
{
    if (regWin)
        regWin->hide();

    if (!loginWin) {
        loginWin = new LoginWindow;

        connect(loginWin, &LoginWindow::sendLoginClicked,
            loginLogic, &Login::doLogin);
        connect(loginWin, &LoginWindow::sendRegisterClicked,
            this, &AppController::showRegisterWindow);
    }

    loginWin->show();
}

void AppController::showRegisterWindow()
{
    if (loginWin)
        loginWin->hide();

    if (!regWin) {
        regWin = new RegisterWindow;

        connect(regWin, &RegisterWindow::sendRegisterClicked,
            regLogic, &Register::doRegister);
        connect(regWin, &RegisterWindow::sendReturnClicked,
            this, &AppController::showLoginWindow);
    }

    regWin->show();
}

// ---------- 业务结果 ----------

void AppController::onLoginSuccess(const QJsonObject& info)
{
    QString username = info["username"].toString();
    QString nickname = info["nickname"].toString();
    QString avatar = info["avatar"].toString();

    // 记住"我是谁"，后面聊天、加好友都要用这个 username
    UserSession::GetInstance().setUser(username, nickname, avatar);
    qDebug() << "current user:" << username << nickname;

    // 聊天记录归这个账号，换账号登录不会串
    if (!storage->open(username)) {
        qDebug() << "chat storage open failed, history will not be saved";
    }
    scatWin = new ScatWindow;
    scatWin->setUserInfo(nickname, avatar);
    scatWin->setSettingsInfo(username, nickname, avatar);
    scatWin->setStoragePath(AppPath::dataRoot());

    connect(scatWin, &ScatWindow::sendTextMessage, this, &AppController::onSendTextMessage);
    connect(scatWin, &ScatWindow::requestHistory, this, &AppController::onRequestHistory);
    connect(scatWin, &ScatWindow::sendAddFriendClicked, this, &AppController::showAddFriendWindow);
    connect(scatWin, &ScatWindow::sendNotifyClicked, this, &AppController::onNotifyClicked);
    connect(scatWin, &ScatWindow::sendSaveNickname, this, &AppController::onSaveNickname);
    connect(scatWin, &ScatWindow::sendChangeStorage, this, &AppController::onChangeStorage);
    connect(scatWin, &ScatWindow::sendChangeAvatar, this, &AppController::onChangeAvatar);
    scatWin->show();
    // 登录窗用不着了
    if (loginWin) {
        loginWin->close();
        loginWin->deleteLater();
        loginWin = nullptr;
    }
    if (regWin) {
        regWin->close();
        regWin->deleteLater();
        regWin = nullptr;
    }

    // 主窗口出来了，去拉好友列表
    friendMgr->requestFriendList();

    friendMgr->requestPendingList();

    if (!AvatarUtils::isCached(avatar))
        settingsMgr->downloadAvatar(avatar);
}

void AppController::onLoginFailed(const QString& reason)
{
    QMessageBox::warning(loginWin, "登录失败", reason);
}

void AppController::onRegisterSuccess(const QString& account)
{
    QMessageBox::information(regWin, "注册成功",
        "账号 " + account + " 注册成功，请登录");
    showLoginWindow();
}

void AppController::onRegisterFailed(const QString& reason)
{
    QMessageBox::warning(regWin, "注册失败", reason);
}

// ---------- 网络 ----------

void AppController::onNetError(const QString& msg)
{
    qDebug() << "network error:" << msg;
}

// ---------- 好友 ----------

void AppController::onFriendListReady(const QJsonArray& friends)
{
    if (!scatWin)
        return;

    // 从本地聊天记录里给每个好友补上最后一条消息，填到列表副标题
    QJsonArray withLast;
    for (const QJsonValue& value : friends) {
        QJsonObject obj = value.toObject();

        ChatMessage last;
        if (storage->lastMessage(obj["username"].toString(), last))
            obj["lastMsg"] = last.content;

        QString avatar = obj["avatar"].toString();
        if (!AvatarUtils::isCached(avatar))
            settingsMgr->downloadAvatar(avatar);

        withLast.append(obj);
    }

    scatWin->setFriendList(withLast);
}

void AppController::onFriendListFailed(const QString& reason)
{
    qDebug() << "friend list failed:" << reason;
}

void AppController::onFriendStatusChanged(const QString& username, bool online)
{
    if (scatWin)
        scatWin->updateFriendStatus(username, online);
}


// ---------- 聊天 ----------

void AppController::onSendTextMessage(const QString& to, const QString& content)
{
    chatNet->sendTextMessage(to, content);
}

void AppController::onRequestHistory(const QString& friendId)
{
    if (scatWin)
        scatWin->loadHistory(storage->loadHistory(friendId));
}

void AppController::onMessageSent(const ChatMessage& msg)
{
    // 先存，再显示。自己发的和收到的走同一条路径
    storage->addMessage(msg);

    if (scatWin)
        scatWin->addChatMessage(msg);
}

void AppController::onMessageReceived(const ChatMessage& msg)
{
    storage->addMessage(msg);

    if (!scatWin)
        return;

    scatWin->addChatMessage(msg);

    // 窗口不在最前面才弹通知 —— 用户正盯着聊天窗还弹一下很烦。
    // 通知图标用对方的头像，比系统那个蓝色感叹号好认
    if (!scatWin->isActiveWindow()) {
        notify->showMessage(msg.from,
            scatWin->nicknameOf(msg.from),
            msg.content,
            QIcon(scatWin->avatarOf(msg.from, 64)));
    }
}

void AppController::onChatSendFailed(const QString& reason)
{
    qDebug() << "chat send failed:" << reason;
}
void AppController::showAddFriendWindow()
{
    if (!addFriendWin) {
        addFriendWin = new AddFriendWindow;

        // 窗口 → 逻辑层
        connect(addFriendWin, &AddFriendWindow::sendSearchUser,
            friendMgr, &FriendManager::searchUser);
        connect(addFriendWin, &AddFriendWindow::sendAddFriend,
            friendMgr, &FriendManager::addFriend);

        // 逻辑层 → 窗口
        connect(friendMgr, &FriendManager::searchResult,
            addFriendWin, &AddFriendWindow::showResult);
        connect(friendMgr, &FriendManager::searchNotFound,
            addFriendWin, &AddFriendWindow::showNotFound);
        connect(friendMgr, &FriendManager::addFriendResult,
            addFriendWin, &AddFriendWindow::onAddSent);
    }

    // 每次打开都清掉上次的搜索结果，按钮回到可点的"添加"
    addFriendWin->clearResult();
    addFriendWin->show();
    addFriendWin->raise();
    addFriendWin->activateWindow();
}


// ---------- 好友申请 ----------

void AppController::updatePendingUi()
{
    pendingCount = pendingRequests.size();

    if (scatWin)
        scatWin->setRequestCount(pendingCount);

    if (requestWin)
        requestWin->setRequests(pendingRequests);
}


void AppController::onNotifyClicked()
{
    if (!requestWin) {
        requestWin = new RequestWindow;

        connect(requestWin, &RequestWindow::sendHandleRequest,
            this, &AppController::onHandleRequest);
    }

    // 先用本地缓存把列表填上 —— 窗口一打开就有内容，
    // 不用干等服务端那一个来回
    requestWin->setRequests(pendingRequests);

    requestWin->show();
    requestWin->raise();
    requestWin->activateWindow();

    // 再向服务端要一次，以它为准刷新
    friendMgr->requestPendingList();
}


void AppController::onPendingListReady(const QJsonArray& requests)
{
    qDebug() << "pending list from server:" << requests.size();

    // 服务端的列表是权威，直接覆盖本地缓存
    pendingRequests = requests;
    updatePendingUi();
}

void AppController::onNewRequestArrived(const QString& username,
    const QString& nickname, const QString& avatar)
{
    // 同一个人重复申请时，服务端 friend_request 表因为 uk_pair 唯一约束
    // 只有一行，本地也必须只留一条，否则红点会越加越多跟实际对不上
    for (const QJsonValue& value : pendingRequests) {
        if (value.toObject()["username"].toString() == username)
            return;
    }

    QJsonObject obj;
    obj["username"] = username;
    obj["nickname"] = nickname;
    obj["avatar"] = avatar;
    pendingRequests.append(obj);

    updatePendingUi();
}

void AppController::onHandleRequest(const QString& username, int action)
{
    // 先把这一行的两个按钮锁住，服务端回来之前别让用户连点
    if (requestWin)
        requestWin->setItemBusy(username, true);

    friendMgr->handleRequest(username, action);
}

void AppController::onRequestHandled(bool ok, const QString& username, int action)
{
    Q_UNUSED(action);

    if (!ok) {
        // 失败了把按钮解锁，让用户能重试
        if (requestWin)
            requestWin->setItemBusy(username, false);
        return;
    }

    // 从缓存里摘掉，红点和列表跟着一起更新
    for (int i = 0; i < pendingRequests.size(); ++i) {
        if (pendingRequests.at(i).toObject()["username"].toString() == username) {
            pendingRequests.removeAt(i);
            break;
        }
    }

    updatePendingUi();
}

void AppController::onFriendListChanged()
{
    // 服务端说好友关系变了，重新拉一遍列表。
    // 同意的一方和被同意的一方都会收到这条
    friendMgr->requestFriendList();
}

// ---------- 设置 ----------

void AppController::onSaveNickname(const QString& nickname)
{
    settingsMgr->saveNickname(nickname);
}

void AppController::onNicknameSaved(bool ok, const QString& nickname, const QString& reason)
{
    if (scatWin)
        scatWin->onNicknameSaved(ok, reason);      // 解锁"保存"按钮

    if (!ok) {
        QMessageBox::warning(scatWin, "保存失败", reason);
        return;
    }

    // 本地也跟着更新：左上角名字 + UserSession
    UserSession& session = UserSession::GetInstance();
    session.setUser(session.username(), nickname, session.avatar());

    if (scatWin)
        scatWin->updateMyNickname(nickname);
}

void AppController::onChangeStorage(const QString& dir)
{
    QString me = UserSession::GetInstance().username();

    // 搬之前必须先把数据库关掉。SQLite 开着 WAL 的时候，
    // chat.db 里的数据有一部分还在 -wal 文件里没落盘，
    // 直接拷可能拷到一个残缺的库
    storage->close();

    QString error;
    bool ok = AppPath::moveDataTo(dir, error);

    // 不管搬成功没有，都得把数据库重新打开，
    // 否则后面收发消息全存不进去。dataRoot() 这时已经指向新位置了
    if (!me.isEmpty())
        storage->open(me);

    if (scatWin)
        scatWin->setStoragePath(AppPath::dataRoot());

    if (ok) {
        QMessageBox::information(scatWin, "更改完成",
            "数据已移动到：\n" + AppPath::dataRoot() +
            "\n\n旧目录里的文件没有删除，确认没问题后可以手动清理。");
    }
    else {
        QMessageBox::warning(scatWin, "更改失败", error);
    }
}

void AppController::onChangeAvatar(const QString& filePath)
{
    settingsMgr->uploadAvatar(filePath);
}

void AppController::onAvatarUploaded(bool ok, const QString& avatar, const QString& reason)
{
    if (scatWin)
        scatWin->onAvatarUploaded(ok, reason);      // 解锁"更换头像"按钮

    if (!ok) {
        QMessageBox::warning(scatWin, "上传失败", reason);
        return;
    }

    UserSession& session = UserSession::GetInstance();
    session.setUser(session.username(), session.nickname(), avatar);

    // 服务端已经有这张图了，但本地缓存还没有。下回来一份，
    // 这样所有显示头像的地方走的都是同一套"缓存 + 圆形裁剪"逻辑，
    // 不用为"自己的头像"单开一条特殊路径
    settingsMgr->downloadAvatar(avatar);
}

void AppController::onAvatarDownloaded(const QString& avatar)
{
    if (!scatWin)
        return;

    // 是自己的，刷新左上角和设置页
    if (avatar == UserSession::GetInstance().avatar())
        scatWin->updateMyAvatar(avatar);

    // 是好友的，刷新列表项和聊天窗顶部
    scatWin->refreshAvatar(avatar);
}

void AppController::onNotificationClicked(const QString& peer)
{
    if (!scatWin)
        return;

    // showNormal + raise + activateWindow 是 Windows 上把窗口调到前台的
    // 标准三件套，少一个都可能不灵
    scatWin->showNormal();
    scatWin->raise();
    scatWin->activateWindow();

    scatWin->selectFriend(peer);     // 顺便切到这个人的会话
}

void AppController::onTrayActivated()
{
    if (!scatWin)
        return;

    scatWin->showNormal();
    scatWin->raise();
    scatWin->activateWindow();
}