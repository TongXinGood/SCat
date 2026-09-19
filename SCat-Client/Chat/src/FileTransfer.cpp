#include "../include/FileTransfer.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// socket 待发缓冲超过这么多字节就先别读了，等发出去一些再继续。
// 太小管道会空转，太大等于把整个文件堆进内存 —— 两块正好
static const qint64 kMaxPending = 2LL * FILE_CHUNK_SIZE;

static QString uniquePath(const QString& path)
{
    if (!QFile::exists(path))
        return path;

    QFileInfo info(path);
    QString base = info.completeBaseName();
    QString suffix = info.suffix().isEmpty() ? QString() : "." + info.suffix();

    for (int i = 1; i < 10000; ++i) {
        QString candidate = info.absolutePath() + "/" + base
            + QString("(%1)").arg(i) + suffix;

        if (!QFile::exists(candidate))
            return candidate;
    }

    return path;      // 一万个重名，放弃治疗
}


FileTransfer::FileTransfer(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net), busy(false), file(nullptr),
    sent(0), seq(0), endSent(false)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &FileTransfer::onPacketReceived);
    connect(net, &NetWorkManager::bytesWritten,
        this, &FileTransfer::onBytesWritten);
    connect(net, &NetWorkManager::disconnected,
        this, &FileTransfer::onDisconnected);
}

FileTransfer::~FileTransfer()
{
    closeCurrent();
}

void FileTransfer::addUpload(const QString& taskId, const QString& to,
    const QString& filePath)
{
    if (!net->isConnected()) {
        emit failed(taskId, "未连接到服务器");
        return;
    }

    QFileInfo info(filePath);

    if (!info.exists() || !info.isFile()) {
        emit failed(taskId, "文件不存在");
        return;
    }

    if (info.size() <= 0) {
        emit failed(taskId, "这是个空文件，没法发送");
        return;
    }

    if (info.size() > MAX_FILE_SIZE) {
        emit failed(taskId, "文件超过 500MB，发不了");
        return;
    }

    Task task;
    task.isUpload = true;
    task.taskId = taskId;
    task.to = to;
    task.filePath = filePath;
    task.fileName = info.fileName();
    task.fileSize = info.size();

    queue.enqueue(task);
    startNext();
}

void FileTransfer::addDownload(const QString& taskId, const QString& fileId,
    const QString& fileName, qint64 fileSize, const QString& saveDir)
{
    if (!net->isConnected()) {
        emit failed(taskId, "未连接到服务器");
        return;
    }

    if (fileId.isEmpty() || fileName.isEmpty()) {
        emit failed(taskId, "文件信息不完整");
        return;
    }

    Task task;
    task.isUpload = false;
    task.taskId = taskId;
    task.fileId = fileId;
    task.fileName = fileName;
    task.fileSize = fileSize;
    task.saveDir = saveDir;

    queue.enqueue(task);
    startNext();
}

void FileTransfer::startNext()
{
    // 一次只传一个。正在传就等它完事，队列空了就歇着
    if (busy || queue.isEmpty())
        return;

    current = queue.dequeue();
    busy = true;
    sent = 0;
    seq = 0;
    endSent = false;
    fileId.clear();

    if (current.isUpload)
        startUpload();
    else
        startDownload();
}

void FileTransfer::startUpload()
{
    file = new QFile(current.filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        failCurrent("打不开这个文件，可能正被别的程序占用");
        return;
    }

    QJsonObject obj;
    obj["to"] = current.to;
    obj["fileName"] = current.fileName;
    obj["fileSize"] = current.fileSize;

    net->sendPacket(MSG_FILE_BEGIN_REQ, obj);

    qDebug() << "upload begin:" << current.fileName << current.fileSize << "bytes";
}

void FileTransfer::startDownload()
{
    QDir().mkpath(current.saveDir);

    // 先把最终文件名占下来（重名自动加后缀），但实际写的是 .part。
    // 下完才改名 —— 中途断了不会在人家目录里留一个看着正常其实是残的文件
    current.filePath = uniquePath(current.saveDir + "/" + current.fileName);

    file = new QFile(current.filePath + ".part");
    if (!file->open(QIODevice::WriteOnly)) {
        failCurrent("写不进下载目录，检查一下权限");
        return;
    }

    // 上传时 fileId 要等服务端给，下载时一开始就知道
    fileId = current.fileId;

    QJsonObject obj;
    obj["fileId"] = fileId;
    net->sendPacket(MSG_FILE_PULL_REQ, obj);

    qDebug() << "download begin:" << current.fileName << current.fileSize << "bytes";
}

void FileTransfer::sendChunks()
{
    if (!busy || !current.isUpload || !file || fileId.isEmpty())
        return;

    // 这个 while 是整个文件传输最关键的地方：一块一块地喂，而且只在
    // socket 待发缓冲不太满的时候才喂。要是写成 for 循环把整个文件
    // 一口气 write 出去，Qt 会把几百 MB 全排进内存，程序当场就飞了
    while (net->pendingBytes() < kMaxPending) {
        QByteArray chunk = file->read(FILE_CHUNK_SIZE);

        if (chunk.isEmpty()) {
            // 读不出东西有两种可能：正常读完了，或者读出错了
            // （U 盘拔了、文件被删了、网络盘掉线）。
            // 出错必须当失败处理，否则 END 永远发不出去，任务就卡死了
            if (file->error() != QFileDevice::NoError) {
                failCurrent("读取文件失败：" + file->errorString());
                return;
            }

            break;      // 正常读完了
        }

        QJsonObject obj;
        obj["fileId"] = fileId;
        obj["seq"] = seq++;
        obj["data"] = QString::fromLatin1(chunk.toBase64());

        net->sendPacket(MSG_FILE_CHUNK, obj);

        sent += chunk.size();
        emit progress(current.taskId, sent, current.fileSize);
    }

    // 全读完了就收尾。endSent 是必要的 —— bytesWritten 还会再触发几次，
    // 不挡住会把 END 包重复发出去
    if (!endSent && sent >= current.fileSize) {
        endSent = true;

        QJsonObject obj;
        obj["fileId"] = fileId;
        net->sendPacket(MSG_FILE_END_REQ, obj);

        qDebug() << "upload chunks done:" << current.fileName << sent << "bytes";
    }
}

void FileTransfer::onBytesWritten()
{
    // 缓冲里发出去一些了，接着灌
    sendChunks();
}

void FileTransfer::onPacketReceived(quint16 type, const QJsonObject& obj)
{
    switch (type) {
    case MSG_FILE_BEGIN_RESP:
        if (!busy || !current.isUpload)
            return;

        if (obj["code"].toInt() != ERR_OK) {
            failCurrent(QString("服务端拒绝接收 (code: %1)").arg(obj["code"].toInt()));
            return;
        }

        fileId = obj["fileId"].toString();
        sendChunks();          // 拿到 fileId 就能开始灌数据了
        break;

    case MSG_FILE_END_RESP:
        // fileId 对不上说明是上一个任务的迟到回执，丢掉
        if (!busy || !current.isUpload || obj["fileId"].toString() != fileId)
            return;

        if (obj["code"].toInt() != ERR_OK) {
            failCurrent(QString("服务端校验失败 (code: %1)").arg(obj["code"].toInt()));
            return;
        }

        finishCurrent(fileId);
        break;

    case MSG_FILE_PULL_RESP:
        if (!busy || current.isUpload || obj["fileId"].toString() != fileId)
            return;

        if (obj["code"].toInt() != ERR_OK) {
            failCurrent("文件不存在，可能已经过期被清理了");
            return;
        }

        // 以服务端报的大小为准，本地记的那个可能是旧的
        current.fileSize = obj["fileSize"].toVariant().toLongLong();
        break;

    case MSG_FILE_PULL_CHUNK:
        onPullChunk(obj);
        break;

    case MSG_FILE_PULL_END:
        onPullEnd(obj);
        break;

    default:
        break;      // 不是文件传输的包，不管
    }
}

void FileTransfer::onPullChunk(const QJsonObject& obj)
{
    if (!busy || current.isUpload || !file)
        return;

    if (obj["fileId"].toString() != fileId)
        return;

    QByteArray data = QByteArray::fromBase64(obj["data"].toString().toLatin1());

    if (file->write(data) != data.size()) {
        failCurrent("写入文件失败，可能磁盘满了");
        return;
    }

    sent += data.size();
    emit progress(current.taskId, sent, current.fileSize);
}

void FileTransfer::onPullEnd(const QJsonObject& obj)
{
    if (!busy || current.isUpload || !file)
        return;

    if (obj["fileId"].toString() != fileId)
        return;

    file->flush();
    QString partPath = file->fileName();
    closeCurrent();

    // 收到的字节数跟服务端说的对不上，文件是坏的，别留着骗人
    if (current.fileSize > 0 && sent != current.fileSize) {
        QFile::remove(partPath);
        failCurrent("下载的文件不完整");
        return;
    }

    // .part 改成正式名字。到这一步才算真的下好了
    if (!QFile::rename(partPath, current.filePath)) {
        QFile::remove(partPath);
        failCurrent("保存文件失败");
        return;
    }

    busy = false;

    qDebug() << "download finished:" << current.filePath;

    emit downloaded(current.taskId, current.filePath);
    startNext();
}

void FileTransfer::cancel(const QString& taskId)
{
    // 还在排队的，直接从队列里划掉
    for (int i = 0; i < queue.size(); ++i) {
        if (queue.at(i).taskId != taskId)
            continue;

        queue.removeAt(i);
        emit canceled(taskId);
        return;
    }

    if (!busy || current.taskId != taskId)
        return;

    // 正在传的，告诉服务端把半截文件删掉
    if (!fileId.isEmpty()) {
        QJsonObject obj;
        obj["fileId"] = fileId;
        net->sendPacket(MSG_FILE_CANCEL, obj);
    }

    discardPart();       // 下载才有 .part 要删，上传时它什么都不做
    closeCurrent();
    busy = false;

    emit canceled(taskId);
    startNext();
}

void FileTransfer::closeCurrent()
{
    if (!file)
        return;

    file->close();
    delete file;
    file = nullptr;
}

void FileTransfer::finishCurrent(const QString& id)
{
    closeCurrent();
    busy = false;

    qDebug() << "upload finished:" << current.fileName;

    emit finished(current.taskId, id);
    startNext();
}

void FileTransfer::discardPart()
{
    // 下载失败或取消时，把没下完的 .part 删掉，
    // 别在人家目录里留个残废文件
    if (current.isUpload || !file)
        return;

    QString partPath = file->fileName();
    closeCurrent();
    QFile::remove(partPath);
}

void FileTransfer::failCurrent(const QString& reason)
{
    discardPart();
    closeCurrent();
    busy = false;

    qDebug() << "upload failed:" << current.fileName << reason;

    emit failed(current.taskId, reason);
    startNext();
}

void FileTransfer::onDisconnected()
{
    // 先清队列再收尾。反过来的话 failCurrent() 末尾会调 startNext()，
    // 把队列里的下一个也开起来，而那时连接已经断了，它会立刻又卡住
    while (!queue.isEmpty()) {
        Task task = queue.dequeue();
        emit failed(task.taskId, "连接断开，传输中止");
    }

    if (busy)
        failCurrent("连接断开，传输中止");
}