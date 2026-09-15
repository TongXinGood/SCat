#ifndef APPPATH_H
#define APPPATH_H

#include <QString>

// 全项目所有本地文件的路径都从这里出，别的地方不要自己拼路径。
// 以后要支持"用户自定义存储位置"，只改 dataRoot() 一个函数就够了
namespace AppPath
{
    // 数据根目录
    QString dataRoot();

    // 某个账号的数据目录，不存在会自动创建
    QString userDir(const QString& user);

    // 某个账号的聊天数据库文件
    QString chatDbPath(const QString& user);

    // 这个目录能不能写
    bool isWritable(const QString& dir);
}

#endif // APPPATH_H