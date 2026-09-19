#include "../include/FileTransfer.h"
#include <QFileInfo>
#include <QDebug>

// socket 待发缓冲超过这么多字节就先别读了，等发出去一些再继续。
// 太小管道会空转，太大等于把整个文件堆进内存 —— 两块正好
static const qint64 kMaxPending = 2LL * FILE_CHUNK_SIZE;

FileTransfer::FileTransfer(NetWorkManager* net, QObject* parent)
    : QObject(parent), net(net), busy(false), file(nullptr),
    sent(0), seq(0), endSent(false)
{
    connect(net, &NetWorkManager::packetReceived,
        this, &FileTransfer::onPacketReceived);
    connect(net, &NetWorkManager::bytesWritten,
        this, &FileTransfer::onBytesWritten);
}

FileTransfer::~FileTransfer()
{
    closeCurrent();
}

void FileTransfer::addUpload(const QString& taskId, const QString& to,
    const QString& filePath)
{
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
    task.taskId = taskId;
    task.to = to;
    task.filePath = filePath;
    task.fileName = info.fileName();
    task.fileSize = info.size();

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

void FileTransfer::sendChunks()
{
    if (!busy || !file || fileId.isEmpty())
        return;

    // 这个 while 是整个文件传输最关键的地方：一块一块地喂，而且只在
    // socket 待发缓冲不太满的时候才喂。要是写成 for 循环把整个文件
    // 一口气 write 出去，Qt 会把几百 MB 全排进内存，程序当场就飞了
    while (net->pendingBytes() < kMaxPending) {
        QByteArray chunk = file->read(FILE_CHUNK_SIZE);

        if (chunk.isEmpty())
            break;      // 读完了

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
        if (!busy)
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
        if (!busy || obj["fileId"].toString() != fileId)
            return;

        if (obj["code"].toInt() != ERR_OK) {
            failCurrent(QString("服务端校验失败 (code: %1)").arg(obj["code"].toInt()));
            return;
        }

        finishCurrent(fileId);
        break;

    default:
        break;      // 不是文件传输的包，不管
    }
}

void FileTransfer::cancel(const QString& taskId)
{
    // 还在排队的，直接从队列里划掉
    for (int i = 0; i < queue.size(); ++i) {
        if (queue.at(i).taskId != taskId)
            continue;

        queue.removeAt(i);
        emit failed(taskId, "已取消");
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

    closeCurrent();
    busy = false;

    emit failed(taskId, "已取消");
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

void FileTransfer::failCurrent(const QString& reason)
{
    closeCurrent();
    busy = false;

    qDebug() << "upload failed:" << current.fileName << reason;

    emit failed(current.taskId, reason);
    startNext();
}