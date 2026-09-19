#include "../include/Server.h"
#include "../include/ClientSession.h"
#include "../../Database/include/Database.h"
#include <QDebug>
#include <QJsonArray>
#include <QDateTime>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

static QString avatarDir()
{
    QString dir = QCoreApplication::applicationDirPath() + "/avatars";
    QDir().mkpath(dir);
    return dir;
}

static QString fileDir()
{
    QString dir = QCoreApplication::applicationDirPath() + "/files";
    QDir().mkpath(dir);
    return dir;
}

void Server::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket* sock = server->nextPendingConnection();

        ClientSession* session = new ClientSession(sock, this);
        sessions.append(session);

        connect(session, &ClientSession::packetReceived,
            this, &Server::onPacketReceived);
        connect(session, &ClientSession::closed,
            this, &Server::onSessionClosed);

        qDebug() << "new client:" << session->peerInfo()
            << "| online:" << sessions.size();
    }
}

void Server::onPacketReceived(ClientSession* from, quint16 type, const QJsonObject& obj)
{
    if (type != MSG_FILE_CHUNK)
        qDebug() << "recv from" << from->peerInfo() << "type" << type;

    switch (type) {
    case MSG_HEARTBEAT:
        from->sendPacket(MSG_HEARTBEAT_RESP, QJsonObject());
        break;

    case MSG_LOGIN_REQ:
        handleLogin(from, obj);
        break;

    case MSG_REG_REQ:
        handleRegister(from, obj);
        break;

    case MSG_FRIEND_LIST_REQ:
        handleFriendList(from);
        break;

    case MSG_CHAT_REQ:
        handleChat(from, obj);
        break;

    case MSG_OFFLINE_ACK:
        handleOfflineAck(from, obj);
        break;

    case MSG_SEARCH_USER_REQ:
        handleSearchUser(from, obj);
        break;

    case MSG_FRIEND_ADD_REQ:
        handleFriendAdd(from, obj);
        break;

    case MSG_FRIEND_REQ_LIST_REQ:
        handleFriendReqList(from);
        break;

    case MSG_FRIEND_HANDLE_REQ:
        handleFriendHandle(from, obj);
        break;

    case MSG_SET_NICKNAME_REQ:
        handleSetNickname(from, obj);
        break;

    case MSG_SET_AVATAR_REQ:
        handleSetAvatar(from, obj);
        break;

    case MSG_GET_AVATAR_REQ:
        handleGetAvatar(from, obj);
        break;

    case MSG_FILE_BEGIN_REQ:
        handleFileBegin(from, obj);
        break;

    case MSG_FILE_CHUNK:
        handleFileChunk(from, obj);
        break;

    case MSG_FILE_END_REQ:
        handleFileEnd(from, obj);
        break;

    case MSG_FILE_CANCEL:
        handleFileCancel(from, obj);
        break;

    default:
        qDebug() << "unknown type:" << type;
        break;
    }
}

void Server::onSessionClosed(ClientSession* session)
{
    // 这条连接上没传完的上传全都作废。不清的话文件句柄一直开着，
    // 半截文件也会一直躺在盘上占地方
    const QStringList ids = uploads.keys();
    for (const QString& id : ids) {
        if (uploads.value(id).from == session)
            abortUpload(id);
    }

    if (session->isLogined()) {
        // 只有在线表里记的确实是这条连接时才移除。
        // 防止以后改成"顶号"策略时，旧连接断开把新连接误删掉。
        if (onlineUsers.value(session->username()) == session)
            onlineUsers.remove(session->username());

        qDebug() << "user offline:" << session->username();

        // 注意顺序：先从在线表移除再通知，否则会给自己也推一条
        notifyFriendsStatus(session->username(), false);
    }

    sessions.removeOne(session);
    session->deleteLater();

    qDebug() << "session removed | online users:" << onlineUsers.size()
        << "| connections:" << sessions.size();
}

Server::Server(Database* db, QObject* parent)
    : QObject(parent), db(db)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection,
        this, &Server::onNewConnection);
}

bool Server::start(quint16 port)
{
    if (!server->listen(QHostAddress::Any, port)) {
        qDebug() << "listen failed:" << server->errorString();
        return false;
    }
    qDebug() << "server listening on port" << port;
    return true;
}

void Server::handleLogin(ClientSession* from, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();

    QJsonObject resp;

    // 同一条连接重复发登录包，直接拒绝，避免在线表里出现脏数据
    if (from->isLogined()) {
        qDebug() << "duplicate login on same connection:" << from->peerInfo();
        resp["code"] = ERR_ALREADY_ONLINE;
        from->sendPacket(MSG_LOGIN_RESP, resp);
        return;
    }

    int code = db->checkLogin(username, password);

    // 账号密码都没问题，再查在线表：已经在别处登录就拒绝这次登录
    if (code == ERR_OK && onlineUsers.contains(username))
        code = ERR_ALREADY_ONLINE;

    resp["code"] = code;

    if (code == ERR_OK) {
        QString nickname, avatar;
        db->getUserInfo(username, nickname, avatar);

        // 绑定：这条连接 = 这个用户。后面转发聊天消息全靠它
        from->setUser(username);
        onlineUsers.insert(username, from);

        resp["username"] = username;
        resp["nickname"] = nickname;
        resp["avatar"] = avatar;

        qDebug() << "login success:" << username
            << "| online users:" << onlineUsers.size();
    }
    else {
        qDebug() << "login failed:" << username << "code:" << code;
    }

    from->sendPacket(MSG_LOGIN_RESP, resp);

    if (code == ERR_OK) {
        notifyFriendsStatus(from->username(), true);
        //补发他不在线时别人发来的消息
        sendOfflineMessages(from);
    }
}

void Server::handleRegister(ClientSession* from, const QJsonObject& obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();
    QString nickname = obj["nickname"].toString().trimmed();

    static const QRegularExpression reName("^[a-zA-Z0-9]{4,15}$");
    if (!reName.match(username).hasMatch()) {
        QJsonObject resp;
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_REG_RESP, resp);
        qDebug() << "register rejected, bad username:" << username;
        return;
    }

    // 密码长度也查一下，跟客户端的规则保持一致
    if (password.length() < 8 || password.length() > 16) {
        QJsonObject resp;
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_REG_RESP, resp);
        qDebug() << "register rejected, bad password length:" << username;
        return;
    }

    if (nickname.isEmpty())
        nickname = username;

    int code = db->addUser(username, password, nickname);

    QJsonObject resp;
    resp["code"] = code;

    if (code == ERR_OK)
        qDebug() << "register success:" << username << nickname;
    else
        qDebug() << "register failed:" << username << "code:" << code;

    from->sendPacket(MSG_REG_RESP, resp);
}

void Server::handleFriendList(ClientSession* from)
{
    QJsonObject resp;

    // 没登录的连接不给查。这就是第 1 步把身份绑到连接上的好处：
    // 客户端不用传自己是谁，也伪造不了别人的身份
    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FRIEND_LIST_RESP, resp);
        qDebug() << "friend list rejected, not logged in:" << from->peerInfo();
        return;
    }

    QList<FriendInfo> list;
    if (!db->getFriendList(from->username(), list)) {
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_FRIEND_LIST_RESP, resp);
        return;
    }

    QJsonArray arr;
    for (const FriendInfo& info : list) {
        QJsonObject obj;
        obj["username"] = info.username;
        obj["nickname"] = info.nickname;
        obj["avatar"] = info.avatar;
        // 在线状态直接查内存里的在线表，不用碰数据库
        obj["online"] = onlineUsers.contains(info.username);
        arr.append(obj);
    }

    resp["code"] = ERR_OK;
    resp["friends"] = arr;

    from->sendPacket(MSG_FRIEND_LIST_RESP, resp);

    qDebug() << "friend list sent to" << from->username()
        << ":" << arr.size() << "friends";
}

void Server::notifyFriendsStatus(const QString& username, bool online)
{
    QList<FriendInfo> list;
    if (!db->getFriendList(username, list))
        return;

    QJsonObject push;
    push["username"] = username;
    push["online"] = online;

    int sent = 0;
    for (const FriendInfo& info : list) {
        // 只推给当前在线的好友。不在线的不用管，
        // 他下次登录时拉好友列表就能拿到最新状态
        ClientSession* session = onlineUsers.value(info.username, nullptr);
        if (session) {
            session->sendPacket(MSG_FRIEND_STATUS_PUSH, push);
            ++sent;
        }
    }

    qDebug() << "status push:" << username << (online ? "online" : "offline")
        << "->" << sent << "friends";
}

void Server::handleChat(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_CHAT_RESP, resp);
        return;
    }

    QString to = obj["to"].toString();
    QString content = obj["content"].toString();

    int kind = obj["kind"].toInt();
    QString image = obj["image"].toString();


    if (to.isEmpty() || content.isEmpty()) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_CHAT_RESP, resp);
        return;
    }

    if (kind == KIND_IMAGE && image.isEmpty()) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_CHAT_RESP, resp);
        return;
    }

    // 时间戳统一由服务端生成。两台电脑的系统时间可能对不上，
    // 各自取本地时间会导致同一段对话在两边的顺序不一样
    qint64 time = QDateTime::currentMSecsSinceEpoch();

    // 全局唯一的消息 ID，也由服务端生成，保证收发双方存的是同一个值。
    // 本地去重、以后的撤回和已读回执都靠它
    QString msgid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 查在线表，找到对方的连接就推过去
    ClientSession* target = onlineUsers.value(to, nullptr);
    if (target) {
        QJsonObject push;
        push["msgid"] = msgid;
        push["from"] = from->username();
        push["to"] = to;
        push["content"] = content;
        push["time"] = time;
        push["kind"] = kind;

        // 图片原样透传，服务端不解码也不落盘。
        // 在线投递完这张图在服务端就不存在了，零存储成本
        if (kind == KIND_IMAGE) {
            push["image"] = image;
            push["w"] = obj["w"].toInt();
            push["h"] = obj["h"].toInt();
        }

        target->sendPacket(MSG_CHAT_PUSH, push);

        qDebug() << "chat" << from->username() << "->" << to
            << ":" << content.left(20);
    }
    else {
        // 对方不在线，先存进 offline_msg 表，等他登录时补发。
        // 图片也一样躺在表里，客户端确认收到后会被删掉，不会长期占地方
        OfflineMsg m;
        m.msgid = msgid;
        m.sender = from->username();
        m.receiver = to;
        m.content = content;
        m.time = time;
        m.kind = kind;
        m.image = image;
        m.imgW = obj["w"].toInt();
        m.imgH = obj["h"].toInt();

        if (db->addOfflineMsg(m))
            qDebug() << "chat stored offline:" << from->username() << "->" << to;
        else
            qDebug() << "store offline failed:" << to;
    }
    // 回执给发送方，带上服务端时间戳，让它拿去存本地
    resp["code"] = ERR_OK;
    resp["msgid"] = msgid;
    resp["to"] = to;
    resp["content"] = content;
    resp["time"] = time;
    resp["kind"] = kind;
    resp["delivered"] = (target != nullptr);

    from->sendPacket(MSG_CHAT_RESP, resp);
}

void Server::sendOfflineMessages(ClientSession* to)
{
    if (!to->isLogined())
        return;

    QList<OfflineMsg> list;

    // 一次最多取 100 条。这批被客户端确认、服务端删掉之后，
    // handleOfflineAck 会再调一次这个函数取下一批
    if (!db->getOfflineMsgs(to->username(), list, 100) || list.isEmpty())
        return;

    // 光靠"一次 100 条"挡不住图片：一条图片 base64 之后有几百 KB，
    // 100 条堆一个包里早就超过 1MB 的包体上限，客户端收到会直接掐断连接。
    // 所以这里边装边算字节数，超了就先发这一批，
    // 剩下的等客户端 ACK 之后 handleOfflineAck 会再调一次本函数继续补发
    static const int kMaxBatchBytes = 600 * 1024;

    QJsonArray arr;
    int batchBytes = 0;

    for (const OfflineMsg& m : list) {
        // 已经装了东西，再装这条就要超了 —— 留到下一批。
        // 判断里的 !arr.isEmpty() 是保险：万一单条就超了阈值，
        // 也得让它单独成一批发出去，否则会卡死在这一条上永远发不完
        if (!arr.isEmpty() && batchBytes + m.image.size() > kMaxBatchBytes)
            break;

        QJsonObject obj;
        obj["msgid"] = m.msgid;
        obj["from"] = m.sender;
        obj["to"] = m.receiver;
        obj["content"] = m.content;
        obj["time"] = m.time;
        obj["kind"] = m.kind;

        if (m.kind == KIND_IMAGE) {
            obj["image"] = m.image;
            obj["w"] = m.imgW;
            obj["h"] = m.imgH;
        }

        batchBytes += m.image.size();
        arr.append(obj);
    }

    QJsonObject push;
    push["msgs"] = arr;
    to->sendPacket(MSG_OFFLINE_PUSH, push);

    qDebug() << "offline messages sent to" << to->username() << ":" << arr.size();
}

void Server::handleOfflineAck(ClientSession* from, const QJsonObject& obj)
{
    if (!from->isLogined())
        return;

    QStringList msgids;
    for (const QJsonValue& value : obj["msgids"].toArray())
        msgids << value.toString();

    if (msgids.isEmpty())
        return;

    // 客户端确认存好了，现在才能删。
    // 要是这个确认包在路上丢了，这批消息会留在表里，下次登录重复投递 ——
    // 客户端本地 msgid 有 UNIQUE 约束会自动去重，宁可重复也不能丢
    db->deleteOfflineMsgs(from->username(), msgids);

    qDebug() << "offline acked by" << from->username() << ":" << msgids.size();

    // 可能还有下一批，接着发。发完了 getOfflineMsgs 返回空，循环自然停下
    sendOfflineMessages(from);
}

void Server::handleSearchUser(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_SEARCH_USER_RESP, resp);
        return;
    }

    QString target = obj["username"].toString().trimmed();
    if (target.isEmpty()) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_SEARCH_USER_RESP, resp);
        return;
    }

    FriendInfo info;
    if (!db->findUser(target, info)) {
        resp["code"] = ERR_USER_NOT_FOUND;
        from->sendPacket(MSG_SEARCH_USER_RESP, resp);
        qDebug() << "search:" << from->username() << "->" << target << "not found";
        return;
    }

    // 告诉客户端这人跟他什么关系，决定"添加"按钮是可点还是灰的。
    // 有没有待处理的申请这里不查 —— 允许重复申请，反正 ON DUPLICATE KEY
    // 会把旧记录重置，不会产生重复行
    int relation = RELATION_STRANGER;
    if (info.username == from->username())
        relation = RELATION_SELF;
    else if (db->isFriend(from->username(), info.username))
        relation = RELATION_FRIEND;

    resp["code"] = ERR_OK;
    resp["username"] = info.username;
    resp["nickname"] = info.nickname;
    resp["avatar"] = info.avatar;
    resp["relation"] = relation;

    from->sendPacket(MSG_SEARCH_USER_RESP, resp);

    qDebug() << "search:" << from->username() << "->" << target
        << "| relation" << relation;
}

void Server::handleFriendAdd(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FRIEND_ADD_RESP, resp);
        return;
    }

    QString me = from->username();
    QString target = obj["username"].toString().trimmed();

    int code = ERR_OK;

    if (target.isEmpty()) {
        code = ERR_INVALID_PARAM;
    }
    else if (target == me) {
        code = ERR_CANNOT_ADD_SELF;
    }
    else {
        FriendInfo info;
        if (!db->findUser(target, info))
            code = ERR_USER_NOT_FOUND;
        else if (db->isFriend(me, target))
            code = ERR_ALREADY_FRIEND;
        else if (!db->addFriendRequest(me, target))
            code = ERR_DB_ERROR;
    }

    resp["code"] = code;
    resp["username"] = target;
    from->sendPacket(MSG_FRIEND_ADD_RESP, resp);

    if (code == ERR_OK) {
        qDebug() << "friend request:" << me << "->" << target;

        // 对方在线就立刻推一条，他的铃铛会当场亮起来。
        // 不在线也不用管，他登录时会主动拉一次待处理列表
        ClientSession* targetSession = onlineUsers.value(target, nullptr);
        if (targetSession) {
            FriendInfo mine;
            db->findUser(me, mine);

            QJsonObject push;
            push["username"] = mine.username;
            push["nickname"] = mine.nickname;
            push["avatar"] = mine.avatar;
            targetSession->sendPacket(MSG_FRIEND_REQ_PUSH, push);
        }
    }
    else {
        qDebug() << "friend request rejected:" << me << "->" << target
            << "| code" << code;
    }
}

void Server::handleFriendReqList(ClientSession* from)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FRIEND_REQ_LIST_RESP, resp);
        return;
    }

    QList<FriendInfo> list;
    if (!db->getPendingRequests(from->username(), list)) {
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_FRIEND_REQ_LIST_RESP, resp);
        return;
    }

    QJsonArray arr;
    for (const FriendInfo& info : list) {
        QJsonObject obj;
        obj["username"] = info.username;
        obj["nickname"] = info.nickname;
        obj["avatar"] = info.avatar;
        arr.append(obj);
    }

    resp["code"] = ERR_OK;
    resp["requests"] = arr;
    from->sendPacket(MSG_FRIEND_REQ_LIST_RESP, resp);

    qDebug() << "request list sent to" << from->username() << ":" << arr.size();
}

void Server::handleFriendHandle(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FRIEND_HANDLE_RESP, resp);
        return;
    }

    QString me = from->username();
    QString sender = obj["username"].toString().trimmed();
    int action = obj["action"].toInt();

    if (sender.isEmpty() || (action != HANDLE_ACCEPT && action != HANDLE_REJECT)) {
        resp["code"] = ERR_INVALID_PARAM;
        resp["username"] = sender;
        from->sendPacket(MSG_FRIEND_HANDLE_RESP, resp);
        return;
    }

    int code = db->handleRequest(me, sender, action);

    resp["code"] = code;
    resp["username"] = sender;
    resp["action"] = action;
    from->sendPacket(MSG_FRIEND_HANDLE_RESP, resp);

    if (code != ERR_OK) {
        qDebug() << "handle request failed:" << me << sender << "| code" << code;
        return;
    }

    if (action == HANDLE_ACCEPT) {
        // 双方的好友列表都变了，各推一条。收到的客户端会重新拉一遍列表 ——
        // 不传具体内容，省了一整套增量更新的逻辑，也不会出现状态不一致
        notifyFriendListChanged(me);
        notifyFriendListChanged(sender);
        qDebug() << "friend accepted:" << me << "<->" << sender;
    }
    else {
        // 拒绝不通知申请方，他那边永远显示"等待验证"
        qDebug() << "friend rejected:" << me << "x" << sender;
    }
}

void Server::notifyFriendListChanged(const QString& username)
{
    ClientSession* session = onlineUsers.value(username, nullptr);
    if (session)
        session->sendPacket(MSG_FRIEND_LIST_CHANGED, QJsonObject());
}

void Server::handleSetNickname(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_SET_NICKNAME_RESP, resp);
        return;
    }

    QString nickname = obj["nickname"].toString().trimmed();

    // 昵称允许中文和各种字符，只限长度。
    // 数据库字段是 VARCHAR(16)，超了会被 MySQL 截断，所以这里先挡住
    if (nickname.isEmpty() || nickname.length() > 16) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_SET_NICKNAME_RESP, resp);
        return;
    }

    if (!db->setNickname(from->username(), nickname)) {
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_SET_NICKNAME_RESP, resp);
        return;
    }

    resp["code"] = ERR_OK;
    resp["nickname"] = nickname;
    from->sendPacket(MSG_SET_NICKNAME_RESP, resp);

    qDebug() << "nickname changed:" << from->username() << "->" << nickname;

    // 告诉在线的好友我的资料变了
    notifyFriendsProfileChanged(from->username());
}

void Server::notifyFriendsProfileChanged(const QString& username)
{
    QList<FriendInfo> list;
    if (!db->getFriendList(username, list))
        return;

    // 直接复用第 6 步那条"好友列表变了"的推送 —— 好友收到就重新拉一遍列表，
    // 新昵称自然就出来了，不用再单独设计一套"资料更新"的包
    for (const FriendInfo& info : list)
        notifyFriendListChanged(info.username);
}

void Server::handleSetAvatar(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_SET_AVATAR_RESP, resp);
        return;
    }

    QByteArray data = QByteArray::fromBase64(obj["data"].toString().toLatin1());
    if (data.isEmpty()) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_SET_AVATAR_RESP, resp);
        return;
    }

    QString me = from->username();

    // 文件名带时间戳当版本号 —— 好友本地缓存里没有这个新文件名，
    // 自然就会重新下载。不用再单独设计一套缓存失效逻辑
    QString fileName = QString("%1_%2.png")
        .arg(me).arg(QDateTime::currentMSecsSinceEpoch());

    QString path = avatarDir() + "/" + fileName;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qDebug() << "save avatar failed:" << path;
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_SET_AVATAR_RESP, resp);
        return;
    }
    f.write(data);
    f.close();

    // 先把旧文件名记下来，等数据库更新成功了再删旧文件
    QString nickname, oldAvatar;
    db->getUserInfo(me, nickname, oldAvatar);

    if (!db->setAvatar(me, fileName)) {
        QFile::remove(path);      // 数据库没写进去，刚落盘的文件也别留
        resp["code"] = ERR_DB_ERROR;
        from->sendPacket(MSG_SET_AVATAR_RESP, resp);
        return;
    }

    // 删掉上一张，不然一个人换十次就堆十个文件。
    // 出厂默认的 head.png 是客户端内置资源，不在这个目录里，不能删
    if (!oldAvatar.isEmpty() && oldAvatar != "head.png" && oldAvatar != fileName)
        QFile::remove(avatarDir() + "/" + oldAvatar);

    resp["code"] = ERR_OK;
    resp["avatar"] = fileName;
    from->sendPacket(MSG_SET_AVATAR_RESP, resp);

    qDebug() << "avatar changed:" << me << "->" << fileName
        << data.size() << "bytes";

    // 好友那边重新拉列表就能拿到新文件名，进而触发下载
    notifyFriendsProfileChanged(me);
}

void Server::handleGetAvatar(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_GET_AVATAR_RESP, resp);
        return;
    }

    QString fileName = obj["avatar"].toString().trimmed();
    resp["avatar"] = fileName;

    // 只接受纯文件名。挡住 "../../xxx" 这种想跳出目录读别的文件的路径 ——
    // 这不是什么高级安全措施，就两行，但能防止服务端被当成文件下载器
    if (fileName.isEmpty() || fileName.contains('/')
        || fileName.contains('\\') || fileName.contains("..")) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_GET_AVATAR_RESP, resp);
        return;
    }

    QFile f(avatarDir() + "/" + fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        resp["code"] = ERR_USER_NOT_FOUND;      // 文件没了，客户端会退回默认头像
        from->sendPacket(MSG_GET_AVATAR_RESP, resp);
        return;
    }

    QByteArray data = f.readAll();
    f.close();

    resp["code"] = ERR_OK;
    resp["data"] = QString::fromLatin1(data.toBase64());
    from->sendPacket(MSG_GET_AVATAR_RESP, resp);

    qDebug() << "avatar sent:" << fileName << data.size() << "bytes";
}

// ---------- 文件上传 ----------

void Server::handleFileBegin(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;

    if (!from->isLogined()) {
        resp["code"] = ERR_NOT_LOGIN;
        from->sendPacket(MSG_FILE_BEGIN_RESP, resp);
        return;
    }

    QString to = obj["to"].toString();
    QString fileName = obj["fileName"].toString();
    qint64 fileSize = obj["fileSize"].toVariant().toLongLong();

    if (to.isEmpty() || fileName.isEmpty() || fileSize <= 0) {
        resp["code"] = ERR_INVALID_PARAM;
        from->sendPacket(MSG_FILE_BEGIN_RESP, resp);
        return;
    }

    if (fileSize > MAX_FILE_SIZE) {
        resp["code"] = ERR_FILE_TOO_LARGE;
        from->sendPacket(MSG_FILE_BEGIN_RESP, resp);
        return;
    }

    // 落盘用 fileId 当文件名，原始文件名只进数据库。
    // 这样路径穿越彻底没戏，不同的人传同名文件也不会互相覆盖
    QString fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString path = fileDir() + "/" + fileId;

    QFile* f = new QFile(path);
    if (!f->open(QIODevice::WriteOnly)) {
        delete f;
        qDebug() << "open file for write failed:" << path;

        resp["code"] = ERR_FILE_IO;
        from->sendPacket(MSG_FILE_BEGIN_RESP, resp);
        return;
    }

    Upload up;
    up.file = f;
    up.from = from;
    up.receiver = to;
    up.fileName = fileName;
    up.fileSize = fileSize;
    uploads.insert(fileId, up);

    db->addFile(fileId, from->username(), to, fileName, fileSize);

    resp["code"] = ERR_OK;
    resp["fileId"] = fileId;
    from->sendPacket(MSG_FILE_BEGIN_RESP, resp);

    qDebug() << "file upload begin:" << fileName << fileSize << "bytes"
        << from->username() << "->" << to;
}

void Server::handleFileChunk(ClientSession* from, const QJsonObject& obj)
{
    QString fileId = obj["fileId"].toString();

    auto it = uploads.find(fileId);
    if (it == uploads.end())
        return;      // 没这个上传，或者刚被取消了，这块静默丢掉

    Upload& up = it.value();

    // 必须是发起这次上传的那条连接，别人不能往里塞东西
    if (up.from != from)
        return;

    QByteArray data = QByteArray::fromBase64(obj["data"].toString().toLatin1());

    if (up.file->write(data) != data.size()) {
        qDebug() << "file write failed:" << fileId;
        abortUpload(fileId);
        return;
    }

    up.received += data.size();

    // 收到的比声明的还多，说明对面在乱发，直接掐掉
    if (up.received > up.fileSize) {
        qDebug() << "file oversize, abort:" << fileId;
        abortUpload(fileId);
    }
}

void Server::handleFileEnd(ClientSession* from, const QJsonObject& obj)
{
    QJsonObject resp;
    QString fileId = obj["fileId"].toString();
    resp["fileId"] = fileId;

    auto it = uploads.find(fileId);
    if (it == uploads.end() || it.value().from != from) {
        resp["code"] = ERR_FILE_NOT_FOUND;
        from->sendPacket(MSG_FILE_END_RESP, resp);
        return;
    }

    Upload up = it.value();
    uploads.erase(it);

    up.file->close();
    delete up.file;

    // 收到的字节数跟一开始声明的对不上，文件就是坏的，绝不能给对方
    if (up.received != up.fileSize) {
        qDebug() << "file size mismatch:" << up.received
            << "expect" << up.fileSize;

        QFile::remove(fileDir() + "/" + fileId);
        db->deleteFile(fileId);

        resp["code"] = ERR_FILE_BROKEN;
        from->sendPacket(MSG_FILE_END_RESP, resp);
        return;
    }

    db->finishFile(fileId);

    resp["code"] = ERR_OK;
    resp["size"] = up.received;
    from->sendPacket(MSG_FILE_END_RESP, resp);

    qDebug() << "file received:" << up.fileName << up.received << "bytes"
        << from->username() << "->" << up.receiver;

    // 第 4 步再在这里把文件消息推给接收方（在线推 / 不在线存 offline_msg）
}

void Server::handleFileCancel(ClientSession* from, const QJsonObject& obj)
{
    QString fileId = obj["fileId"].toString();

    auto it = uploads.find(fileId);
    if (it == uploads.end() || it.value().from != from)
        return;

    abortUpload(fileId);
}

void Server::abortUpload(const QString& fileId)
{
    auto it = uploads.find(fileId);
    if (it == uploads.end())
        return;

    Upload up = it.value();
    uploads.erase(it);

    up.file->close();
    delete up.file;

    QFile::remove(fileDir() + "/" + fileId);
    db->deleteFile(fileId);

    qDebug() << "upload aborted:" << fileId << up.fileName;
}