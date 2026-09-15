#include "../include/AppController.h"
#include "../include/UserSession.h"
#include <QMessageBox>
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
    net->connectToServer("127.0.0.1", 8888);
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

    connect(scatWin, &ScatWindow::sendTextMessage, this, &AppController::onSendTextMessage);
    connect(scatWin, &ScatWindow::requestHistory, this, &AppController::onRequestHistory);
    connect(scatWin, &ScatWindow::sendAddFriendClicked, this, &AppController::showAddFriendWindow);
    connect(scatWin, &ScatWindow::sendNotifyClicked, this, &AppController::onNotifyClicked);

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

    if (scatWin)
        scatWin->addChatMessage(msg);
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

void AppController::onNotifyClicked()
{
    if (!requestWin) {
        requestWin = new RequestWindow;

        connect(requestWin, &RequestWindow::sendHandleRequest,
            this, &AppController::onHandleRequest);
    }

    requestWin->show();
    requestWin->raise();
    requestWin->activateWindow();

    // 每次打开都重新拉一次，保证看到的是最新的
    friendMgr->requestPendingList();
}

void AppController::onPendingListReady(const QJsonArray& requests)
{
    pendingCount = requests.size();

    if (scatWin)
        scatWin->setRequestCount(pendingCount);

    // 窗口没打开就只更新红点，打开了才刷新列表内容
    if (requestWin)
        requestWin->setRequests(requests);
}

void AppController::onNewRequestArrived(const QString& username,
    const QString& nickname, const QString& avatar)
{
    ++pendingCount;

    if (scatWin)
        scatWin->setRequestCount(pendingCount);

    // 窗口正开着就直接把新的一行插进去，不用等用户重新打开
    if (requestWin && requestWin->isVisible())
        requestWin->addRequest(username, nickname, avatar);
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

    // 处理成功：那一行消失，红点减一
    if (requestWin)
        requestWin->removeRequest(username);

    if (pendingCount > 0)
        --pendingCount;

    if (scatWin)
        scatWin->setRequestCount(pendingCount);
}

void AppController::onFriendListChanged()
{
    // 服务端说好友关系变了，重新拉一遍列表。
    // 同意的一方和被同意的一方都会收到这条
    friendMgr->requestFriendList();
}