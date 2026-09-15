#include "../include/AvatarUtils.h"
#include "../include/AppPath.h"
#include "../include/UserSession.h"
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>
#include <QDir>
#include <QDebug>

namespace AvatarUtils
{

    static const int kAvatarSize = 256;                            // 上传后统一边长
    static const qint64 kMaxSourceSize = 20LL * 1024 * 1024;       // 原图最大 20MB

    QString defaultAvatar()
    {
        return ":/Resource/icon/head.png";
    }

    QString pathFor(const QString& avatar)
    {
        // 空的或者还是出厂默认，走内置资源，不占服务端文件
        if (avatar.isEmpty() || avatar == "head.png")
            return defaultAvatar();

        QString user = UserSession::GetInstance().username();
        if (user.isEmpty())
            return defaultAvatar();

        return AppPath::avatarCachePath(user, avatar);
    }

    bool isCached(const QString& avatar)
    {
        if (avatar.isEmpty() || avatar == "head.png")
            return true;      // 内置资源，永远算"有"

        return QFile::exists(pathFor(avatar));
    }

    bool saveToCache(const QString& avatar, const QByteArray& data)
    {
        if (avatar.isEmpty() || data.isEmpty())
            return false;

        QString path = pathFor(avatar);
        QDir().mkpath(QFileInfo(path).absolutePath());

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            qDebug() << "save avatar cache failed:" << path;
            return false;
        }

        f.write(data);
        f.close();
        return true;
    }

    QPixmap round(const QPixmap& src, int size)
    {
        QPixmap dst(size, size);
        dst.fill(Qt::transparent);

        if (src.isNull())
            return dst;

        QPainter p(&dst);
        p.setRenderHint(QPainter::Antialiasing);            // 不开抗锯齿边缘是锯齿状
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        path.addEllipse(0, 0, size, size);
        p.setClipPath(path);

        // QSS 的 border-radius 只裁背景和边框，裁不了 setPixmap 进去的图，
        // 所以圆形只能在这里自己画。这是 Qt 最经典的坑之一
        QPixmap scaled = src.scaled(size, size,
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        // KeepAspectRatioByExpanding 会超出一点，居中放置把多的裁掉，
        // 这样圆里是填满的，不会有白边
        p.drawPixmap((size - scaled.width()) / 2,
            (size - scaled.height()) / 2, scaled);

        return dst;
    }

    QPixmap load(const QString& avatar, int size)
    {
        QPixmap pix(pathFor(avatar));

        // 缓存还没下下来、或者文件坏了，都退回默认头像，别显示一片空白
        if (pix.isNull())
            pix = QPixmap(defaultAvatar());

        return round(pix, size);
    }

    QByteArray makeUploadData(const QString& filePath, QString& error)
    {
        QFileInfo info(filePath);

        if (!info.exists()) {
            error = "文件不存在";
            return QByteArray();
        }

        // 先看文件大小再决定要不要加载 —— 直接用 QImage 打开一个几百 MB 的图
        // 会把内存吃光
        if (info.size() > kMaxSourceSize) {
            error = "图片太大了，请选择 20MB 以内的图片";
            return QByteArray();
        }

        QImage img(filePath);
        if (img.isNull()) {
            error = "这个文件不是有效的图片";
            return QByteArray();
        }

        // 按最短边居中裁成正方形再缩到 256x256。
        // 不做交互式裁剪框，用户无感
        int side = qMin(img.width(), img.height());
        QImage square = img.copy((img.width() - side) / 2,
            (img.height() - side) / 2, side, side)
            .scaled(kAvatarSize, kAvatarSize,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);

        if (!square.save(&buffer, "PNG")) {
            error = "图片编码失败";
            return QByteArray();
        }

        return data;
    }

}