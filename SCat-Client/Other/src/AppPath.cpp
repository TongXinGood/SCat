#include "../include/AppPath.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QUuid>

namespace AppPath
{

    QString dataRoot()
    {
        // 开发期：放在 exe 旁边的 data/，好找、好删、好备份。
        //
        // 以后打包成安装程序时，这里改成：
        //   1. 先读 QSettings 里用户设置的路径，有就用它
        //   2. 没设置就用 QStandardPaths::DocumentsLocation + "/SCat Files"
        // 不要用安装目录 —— 如果用户装在 C:\Program Files\ 下是写不进去的。
        // 全项目只有这一个函数需要改
        return QCoreApplication::applicationDirPath() + "/data";
    }

    QString userDir(const QString& user)
    {
        QString dir = dataRoot() + "/" + user;
        QDir().mkpath(dir);
        return dir;
    }

    QString chatDbPath(const QString& user)
    {
        return userDir(user) + "/chat.db";
    }

    bool isWritable(const QString& dir)
    {
        if (dir.isEmpty() || !QDir().mkpath(dir))
            return false;

        // 不去查 Windows 的权限 API —— 那套规则复杂得离谱，而且查出来
        // "有权限"实际还可能写不进去。真写一个文件试试才靠得住
        QString probe = dir + "/.write_test_"
            + QUuid::createUuid().toString(QUuid::WithoutBraces);

        QFile f(probe);
        if (!f.open(QIODevice::WriteOnly))
            return false;

        f.close();
        f.remove();
        return true;
    }

}