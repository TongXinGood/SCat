#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QObject>
#include <QQueue>
#include <QFile>
#include <QJsonObject>
#include "../../NetWork/include/NetWorkManager.h"
#include "../../Protocol.h"

class NetWorkManager;

// 文件传输的状态机。只管传，不碰界面。
//
// 有两条铁律：
//   1. 状态必须在这里，不能放气泡里 —— QListWidget 的 item widget
//      切会话时会被销毁重建，状态放气泡里切一下就没了
//   2. 串行队列，一次只传一个。同时传几个 500MB 的文件，内存和带宽都会很难看
class FileTransfer : public QObject
{
    Q_OBJECT

public:
    explicit FileTransfer(NetWorkManager* net, QObject* parent = nullptr);
    ~FileTransfer();

    // 排进上传队列。taskId 由调用方生成，用来对应界面上的那个气泡
    void addUpload(const QString& taskId, const QString& to, const QString& filePath);
    void cancel(const QString& taskId);

signals:
    void progress(const QString& taskId, qint64 done, qint64 total);
    void finished(const QString& taskId, const QString& fileId);
    void failed(const QString& taskId, const QString& reason);

private slots:
    void onPacketReceived(quint16 type, const QJsonObject& obj);
    void onBytesWritten();

private:
    void startNext();                          // 队列里还有就开下一个
    void sendChunks();                         // 能塞多少塞多少，塞不下就等
    void finishCurrent(const QString& id);
    void failCurrent(const QString& reason);
    void closeCurrent();

private:
    struct Task
    {
        QString taskId;
        QString to;
        QString filePath;
        QString fileName;
        qint64  fileSize = 0;
    };

    NetWorkManager* net;

    QQueue<Task> queue;        // 排队等着的
    Task current;              // 正在传的
    bool busy;                 // 有没有任务在跑

    QFile* file;               // 正在读的源文件
    QString fileId;            // 服务端给的 ID
    qint64 sent;               // 已经发出去多少字节
    int seq;                   // 块序号，排查问题用
    bool endSent;              // END 包发过没有，防止重复发
};

#endif // FILETRANSFER_H