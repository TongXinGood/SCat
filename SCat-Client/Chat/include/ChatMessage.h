#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QMetaType>
#include "../../Protocol.h" 


enum FileState : int
{
    FILE_STATE_NONE = 0,         // 文本/图片消息，用不上这个字段
    FILE_STATE_SENDING = 1,      // 上传中
    FILE_STATE_SENT = 2,         // 上传完成（自己发的）
    FILE_STATE_READY = 3,        // 别人发来的，还没点下载
    FILE_STATE_DOWNLOADING = 4,  // 下载中
    FILE_STATE_DONE = 5,         // 已经下到本地了
    FILE_STATE_FAILED = 6,       // 失败，可以重试
    FILE_STATE_CANCELED = 7,     // 用户手动取消的
};

// 一条聊天消息。网络层、存储层、界面层都用这个结构传递
struct ChatMessage
{
    QString msgid;          // 服务端生成的全局唯一 ID
    QString from;           // 发送方 username
    QString to;             // 接收方 username
    QString content;        // 文本内容。图片消息固定是 "[图片]"，
    // 左边列表的副标题直接拿它用，不用特判
    qint64  time = 0;       // 服务端生成的毫秒时间戳
    bool    isSelf = false; // 是不是我发的，画气泡时决定摆左边还是右边

    // --- 图片消息才用得上的字段 ---
    int     kind = KIND_TEXT;   // 这条是文本还是图片
    QString imgName;            // 本地文件名（不是完整路径！）如 "xxxx.jpg"。
    // 存文件名是因为用户能改数据存储位置，
    // 存绝对路径的话一改目录历史图片全失效
    int     imgW = 0;           // 图片宽高。画气泡时不用解码图片就能定尺寸，
    int     imgH = 0;           // 切会话滚动才不会卡

    // --- 文件消息才用得上的字段 ---
    QString fileId;             // 服务端给的 UUID，下载时靠它找文件
    QString fileName;           // 原始文件名，显示用
    qint64  fileSize = 0;
    QString filePath;           // 本地完整路径。含义跟 isSelf 走：
    //   自己发的 -> 源文件在哪（重试时要重新读它）
    //   别人发的 -> 下载到了哪，空 = 还没下载
    // 这里存的是绝对路径，所以用之前都要 QFile::exists 确认一下
    int     fileState = FILE_STATE_NONE;

    // 这条消息属于跟谁的对话
    QString peer() const { return isSelf ? to : from; }
};

// 让 ChatMessage 能在信号槽里传递
Q_DECLARE_METATYPE(ChatMessage)

#endif // CHATMESSAGE_H