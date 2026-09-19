#include "../include/AppPath.h"
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>

namespace AppPath
{

    // 配置里存路径用的键名
    static const char* kDataRootKey = "storage/dataRoot";

    QString defaultDataRoot()
    {
        // 出厂默认：exe 旁边的 data 目录
        return QCoreApplication::applicationDirPath() + "/data";
    }

    QString dataRoot()
    {
        // QSettings 在 Windows 上写注册表 HKCU\Software\SCat\SCat-Client，
        // 一定可写、跟安装目录无关，正好用来存"数据目录在哪"这个信息 ——
        // 它本身不能存在可配置的目录里，否则就是鸡生蛋
        QSettings settings;
        QString custom = settings.value(kDataRootKey).toString();

        if (!custom.isEmpty())
            return custom;

        return defaultDataRoot();
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

    // 递归拷贝一个目录，只在本文件内部用
    static bool copyDirInternal(const QString& from, const QString& to, QString& error)
    {
        QDir src(from);
        if (!src.exists())
            return true;      // 源目录不存在，没什么可搬的

        QDir().mkpath(to);

        const QFileInfoList list = src.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo& info : list) {
            QString target = to + "/" + info.fileName();

            if (info.isDir()) {
                if (!copyDirInternal(info.absoluteFilePath(), target, error))
                    return false;
            }
            else {
                // QFile::copy 遇到已存在的目标会直接失败，得先删掉
                if (QFile::exists(target))
                    QFile::remove(target);

                if (!QFile::copy(info.absoluteFilePath(), target)) {
                    error = "复制失败：" + info.fileName();
                    return false;
                }
            }
        }

        return true;
    }

    bool moveDataTo(const QString& newRoot, QString& error)
    {
        QString oldRoot = QDir::cleanPath(dataRoot());
        QString target = QDir::cleanPath(newRoot);

        if (oldRoot == target) {
            error = "新目录和当前目录是同一个";
            return false;
        }

        // 不能搬进自己的子目录，否则边拷边生成，会无限套娃
        if (target.startsWith(oldRoot + "/")) {
            error = "不能选择当前数据目录下面的子目录";
            return false;
        }

        if (!isWritable(target)) {
            error = "这个目录没有写入权限，请换一个";
            return false;
        }

        if (!copyDirInternal(oldRoot, target, error))
            return false;

        QSettings settings;
        settings.setValue(kDataRootKey, target);
        settings.sync();

        // 旧目录故意不删 —— 万一拷贝有问题数据还在，用户能自己找回来。
        // 删用户的数据这种事，程序不该擅自做主
        qDebug() << "data moved:" << oldRoot << "->" << target;
        return true;
    }
    QString avatarCacheDir(const QString& user)
    {
        // 这里不建目录 —— 显示头像时会频繁调用，真正写文件的时候再 mkpath
        return dataRoot() + "/" + user + "/avatars";
    }

    QString avatarCachePath(const QString& user, const QString& fileName)
    {
        return avatarCacheDir(user) + "/" + fileName;
    }

    QString imageDir(const QString& user)
    {
        // 跟 avatarCacheDir 一样不在这里建目录 —— 画气泡时会频繁调用，
        // 真正要写文件的时候再 mkpath
        return dataRoot() + "/" + user + "/images";
    }

    QString imagePath(const QString& user, const QString& fileName)
    {
        return imageDir(user) + "/" + fileName;
    }

    QString fileDir(const QString& user)
    {
        // 跟 imageDir 一样不在这里建目录，真要落盘的时候再 mkpath
        return dataRoot() + "/" + user + "/files";
    }

    QString filePath(const QString& user, const QString& fileName)
    {
        return fileDir(user) + "/" + fileName;
    }
}
