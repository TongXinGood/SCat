#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>

// 聊天图片的公共处理。压缩、落盘、气泡尺寸计算都走这里，
// 想改压缩参数只要动这一个文件。跟 AvatarUtils 是同一个套路
namespace ImageUtils
{
    // 压缩成 JPEG 字节流。长边超了先等比缩小，再按质量逐级降到目标大小以内。
    // outSize 带出压缩后的宽高，失败返回空、error 带原因
    QByteArray compress(const QImage& src, QSize& outSize, QString& error);

    // 从文件读一张图。太大、不存在、不是图片都返回空图，error 带原因
    QImage loadFile(const QString& filePath, QString& error);

    // 光看扩展名判断是不是图片文件。粘贴时要频繁问，不能每次都去真读一遍
    bool isImageFile(const QString& filePath);
   

    // 存进当前账号的 images 目录，返回文件名（不是路径）。失败返回空
    QString saveToLocal(const QByteArray& data);

    // 文件名 -> 完整路径。空的或者文件不在了，都返回空串
    QString pathFor(const QString& imgName);

    // 按气泡的最大显示尺寸等比缩放。原图比框小就不放大，放大了糊
    QSize fitSize(int imgW, int imgH, int maxW, int maxH);
}

#endif // IMAGEUTILS_H