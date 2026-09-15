#ifndef APPPATH_H
#define APPPATH_H

#include <QString>

// 全项目所有本地文件的路径都从这里出，别的地方不要自己拼路径
namespace AppPath
{
    // 数据根目录。先读用户设置的，没有就用默认值
    QString dataRoot();

    // 出厂默认位置（exe 旁边的 data）
    QString defaultDataRoot();

    // 某个账号的数据目录，不存在会自动创建
    QString userDir(const QString& user);

    // 某个账号的聊天数据库文件
    QString chatDbPath(const QString& user);

    QString avatarCacheDir(const QString& user);
    QString avatarCachePath(const QString& user, const QString& fileName);

    // 这个目录能不能写
    bool isWritable(const QString& dir);

    // 把现有数据搬到新目录并记住这个位置。
    // 调用前必须先把数据库关掉，否则 WAL 文件被占用，拷出来可能是残的
    bool moveDataTo(const QString& newRoot, QString& error);
}

#endif // APPPATH_H