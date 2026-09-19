#include "../include/ImageUtils.h"
#include "../include/AppPath.h"
#include "../include/UserSession.h"
#include <QPainter>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUuid>
#include <QImageReader>
#include <QDebug>

namespace ImageUtils
{

    static const int kMaxSide = 1920;                        // 长边上限
    static const int kMaxBytes = 300 * 1024;                 // 压缩后的目标大小
    static const qint64 kMaxSourceSize = 30LL * 1024 * 1024; // 原图最大 30MB

    // 质量逐级往下降。绝大多数图第一档就过了，
    // 后面几档是给超大截图、全景图兜底的
    static const int kQualities[] = { 80, 70, 60, 50 };

    QByteArray compress(const QImage& src, QSize& outSize, QString& error)
    {
        if (src.isNull()) {
            error = "这不是一张有效的图片";
            return QByteArray();
        }

        QImage img = src;

        // 长边超了先等比缩小。两个方向都传 kMaxSide，
        // KeepAspectRatio 会自己挑该按哪一边算
        if (img.width() > kMaxSide || img.height() > kMaxSide)
            img = img.scaled(kMaxSide, kMaxSide,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // JPEG 不支持透明。带 alpha 的图（比如 PNG 截图）直接存，
        // 透明区域会变成黑块，所以先铺一层白底把它填掉
        if (img.hasAlphaChannel()) {
            QImage opaque(img.size(), QImage::Format_RGB32);
            opaque.fill(Qt::white);

            QPainter p(&opaque);
            p.drawImage(0, 0, img);
            p.end();

            img = opaque;
        }

        outSize = img.size();

        QByteArray data;

        for (int quality : kQualities) {
            data.clear();

            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);

            if (!img.save(&buffer, "JPEG", quality)) {
                error = "图片编码失败";
                return QByteArray();
            }

            if (data.size() <= kMaxBytes) {
                qDebug() << "image compressed:" << img.width() << "x" << img.height()
                    << "quality" << quality << data.size() << "bytes";
                return data;
            }
        }

        // 降到最低一档还是超 —— 认了。300KB 只是目标不是硬上限，
        // 离包体上限（1MB）还远着，发得出去就行
        qDebug() << "image still large after compress:" << data.size() << "bytes";
        return data;
    }

    QImage loadFile(const QString& filePath, QString& error)
    {
        QFileInfo info(filePath);

        if (!info.exists()) {
            error = "文件不存在";
            return QImage();
        }

        // 先看文件大小再决定要不要加载 —— 直接用 QImage 打开一个几百 MB 的图
        // 会把内存吃光。跟 AvatarUtils::makeUploadData 一个道理
        if (info.size() > kMaxSourceSize) {
            error = "图片太大了，请选择 30MB 以内的图片";
            return QImage();
        }

        // GIF 会被 QImage 读成第一帧，动图变静态图。
        // 现阶段先这样，支持动图是另一套活（QMovie + 原样传输）
        QImage img(filePath);
        if (img.isNull())
            error = "这个文件不是有效的图片";

        return img;
    }

    bool isImageFile(const QString& filePath)
    {
        QString suffix = QFileInfo(filePath).suffix().toLower();
        if (suffix.isEmpty())
            return false;

        // 拿 Qt 自己支持的格式列表去比，比手写一串扩展名可靠，
        // 平台上装了插件才支持的格式也不会漏掉
        return QImageReader::supportedImageFormats().contains(suffix.toLatin1());
    }

    QString saveToLocal(const QByteArray& data)
    {
        if (data.isEmpty())
            return QString();

        QString user = UserSession::GetInstance().username();
        if (user.isEmpty())
            return QString();

        // 文件名用本地生成的 UUID，不用服务端的 msgid ——
        // 点发送的那一刻还没拿到 msgid，绑死它就得先把图攒在内存里等回执，
        // 平白多一套状态。收发两边各存各的名字，互不影响
        QString fileName = QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jpg";
        QString path = AppPath::imagePath(user, fileName);

        QDir().mkpath(QFileInfo(path).absolutePath());

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            qDebug() << "save image failed:" << path;
            return QString();
        }

        f.write(data);
        f.close();

        qDebug() << "image saved:" << fileName << data.size() << "bytes";
        return fileName;
    }

    QString pathFor(const QString& imgName)
    {
        if (imgName.isEmpty())
            return QString();

        QString user = UserSession::GetInstance().username();
        if (user.isEmpty())
            return QString();

        QString path = AppPath::imagePath(user, imgName);

        // 文件可能被用户手动删了，或者换数据目录时没拷过来。
        // 返回空串，让调用方去显示"图片已失效"，别留一块空白
        return QFile::exists(path) ? path : QString();
    }

    QSize fitSize(int imgW, int imgH, int maxW, int maxH)
    {
        // 宽高没存上（出错了或者是老数据），给个默认框，
        // 别返回 0 让整个气泡塌掉
        if (imgW <= 0 || imgH <= 0)
            return QSize(maxW, maxH);

        // 比框小就原样显示，不往大了拉
        if (imgW <= maxW && imgH <= maxH)
            return QSize(imgW, imgH);

        return QSize(imgW, imgH).scaled(maxW, maxH, Qt::KeepAspectRatio);
    }

}