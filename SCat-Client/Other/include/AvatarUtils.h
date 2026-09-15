#ifndef AVATARUTILS_H
#define AVATARUTILS_H

#include <QPixmap>
#include <QString>
#include <QByteArray>

// 头像相关的公共处理。全项目显示头像都走 load()，
// 想改圆形效果、默认图、缓存位置，只要改这一个文件
namespace AvatarUtils
{
    QString defaultAvatar();                     // 内置默认头像的资源路径

    // avatar 字段 -> 真正能加载的路径。
    // 空或 "head.png" -> 内置资源；否则 -> 本地缓存文件
    QString pathFor(const QString& avatar);

    bool isCached(const QString& avatar);        // 本地有没有这个头像文件
    bool saveToCache(const QString& avatar, const QByteArray& data);

    // 统一加载入口：取路径 + 缩放 + 裁圆 + 找不到就退回默认图
    QPixmap load(const QString& avatar, int size);

    QPixmap round(const QPixmap& src, int size); // 把一张图裁成圆形

    // 用户选的图片文件 -> 256x256 正方形 PNG 字节流，失败返回空
    QByteArray makeUploadData(const QString& filePath, QString& error);
}

#endif // AVATARUTILS_H