#include "../include/EmojiManager.h"
#include <QDir>
#include <QImage>
#include <QStringList>
#include <QDebug>
#include <QPainter>

static const char* kEmojiRoot = ":/Resource/emoji";

// 文件夹名 -> 显示名，顺序就是弹窗里分类的顺序。
// 不在表里的文件夹也会加载，排在最后，显示名直接用文件夹名
static const QList<QPair<QString, QString>> kCategoryNames = {
    { "smileys",  "黄脸" },
    { "gestures", "手势" },
    { "hearts",   "爱心" },
    { "animals",  "动物" },
    { "food",     "食物" },
};

// FE0F 是"按彩色 emoji 显示"的标记。有些字符默认是黑白文字样式，
// 比如 ☺(263A)、↔(2194)，后面不跟 FE0F 的话别的软件会把它画成普通符号。
// BMP 里的字符基本都属于这种，统一补上；BMP 以外的只有少数几个，单独列出来
static const QList<char32_t> kTextStyleWide = {
    0x1F32B,    // 🌫 雾，😶‍🌫️ 里用到
};

static const ushort kVariation = 0xFE0F;
static const ushort kZwj = 0x200D;

EmojiManager& EmojiManager::GetInstance()
{
    // C++11 起，函数内 static 的初始化是线程安全的
    static EmojiManager instance;
    return instance;
}

EmojiManager::EmojiManager() : maxKeyLen(0)
{
    load();
}

void EmojiManager::load()
{
    QStringList folders = QDir(kEmojiRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // 先按表里的顺序加载认识的分类
    for (const auto& pair : kCategoryNames) {
        if (folders.removeOne(pair.first))
            loadCategory(pair.first, pair.second);
    }

    // 剩下不认识的，按文件夹名排在后面
    for (const QString& folder : folders)
        loadCategory(folder, folder);

    qDebug() << "emoji loaded:" << itemById.size() << "in" << cats.size() << "categories";
}

void EmojiManager::loadCategory(const QString& key, const QString& name)
{
    QDir dir(QString("%1/%2").arg(kEmojiRoot, key));

    // 按文件名排序。文件名前面加了 "001_" 这种序号的会按序号排，没加的就按编码排
    QStringList files = dir.entryList({ "*.png" }, QDir::Files, QDir::Name);

    EmojiCategory cat;
    cat.key = key;
    cat.name = name;

    for (const QString& file : files) {
        QString id = parseId(file);
        if (id.isEmpty()) {
            qDebug() << "emoji file name not recognized:" << file;
            continue;
        }

        // 同一个表情放进了两个文件夹，只认第一个，不然匹配时不知道该用哪张
        if (itemById.contains(id))
            continue;

        EmojiItem item;
        item.id = id;
        item.text = idToText(id);
        item.path = dir.filePath(file);

        QString matchKey = stripVariation(item.text);
        idByKey.insert(matchKey, id);
        maxKeyLen = qMax(maxKeyLen, int(matchKey.size()));

        itemById.insert(id, item);
        cat.items.append(item);
    }

    // 空文件夹不显示，免得弹窗里多一个点进去什么都没有的分类
    if (!cat.items.isEmpty())
        cats.append(cat);
}

QString EmojiManager::textOf(const QString& id) const
{
    return itemById.value(id).text;
}

QPixmap EmojiManager::pixmap(const QString& id, int size, qreal dpr)
{
    auto item = itemById.constFind(id);
    if (item == itemById.constEnd())
        return QPixmap();

    QString cacheKey = QString("%1@%2@%3").arg(id).arg(size).arg(dpr);

    auto cached = pixCache.constFind(cacheKey);
    if (cached != pixCache.constEnd())
        return cached.value();

    // 原图 128px，显示只要 20 来个像素，缩小倍数很大。
    // 必须用 SmoothTransformation 先缩好，交给 QPainter 现场缩的话边缘全是锯齿
    int px = qRound(size * dpr);

    QImage img(item->path);
    if (img.isNull())
        return QPixmap();

    QPixmap pix = QPixmap::fromImage(
        img.scaled(px, px, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 告诉 Qt 这张图是按几倍屏准备的，它画的时候会按 size 的逻辑大小摆放
    pix.setDevicePixelRatio(dpr);

    pixCache.insert(cacheKey, pix);
    return pix;
}

QPixmap EmojiManager::inlinePixmap(const QString& id, int size, int padding, qreal dpr)
{
    QString cacheKey = QString("%1@%2@%3@inline%4").arg(id).arg(size).arg(dpr).arg(padding);

    auto cached = pixCache.constFind(cacheKey);
    if (cached != pixCache.constEnd())
        return cached.value();

    QPixmap emoji = pixmap(id, size, dpr);
    if (emoji.isNull())
        return QPixmap();

    QPixmap pix(qRound((size + padding * 2) * dpr), qRound(size * dpr));
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);

    // 两张图的 dpr 一样，这里按逻辑像素画，正好原样贴上去不会再缩一遍
    QPainter painter(&pix);
    painter.drawPixmap(QPointF(padding, 0), emoji);
    painter.end();

    pixCache.insert(cacheKey, pix);
    return pix;
}

int EmojiManager::matchAt(const QString& text, int pos, QString& id) const
{
    if (pos < 0 || pos >= text.size() || text.at(pos).unicode() == kVariation)
        return 0;

    // 一边往后走一边拼 key（跳过 FE0F），每拼一个字符查一次表，记下最长的那次命中。
    // 要找最长的，是因为 🙂 本身是表情，🙂‍↔️ 也是表情，前者是后者的开头
    QString key;
    int bestLen = 0;
    int i = pos;

    while (i < text.size() && key.size() < maxKeyLen) {
        QChar c = text.at(i++);

        if (c.unicode() == kVariation)
            continue;

        key.append(c);

        auto hit = idByKey.constFind(key);
        if (hit != idByKey.constEnd()) {
            bestLen = i - pos;
            id = hit.value();
        }
    }

    if (bestLen == 0)
        return 0;

    // 紧跟在后面的 FE0F 也算这个表情的，不然删表情时会剩下一个看不见的字符
    while (pos + bestLen < text.size() && text.at(pos + bestLen).unicode() == kVariation)
        ++bestLen;

    return bestLen;
}

QString EmojiManager::parseId(const QString& fileName)
{
    // "emoji_u1f600.png" -> "1f600"
    // "001_emoji_u1f600.png" -> "1f600"，前面的序号只管排序
    int start = fileName.indexOf("emoji_u");
    if (start < 0 || !fileName.endsWith(".png", Qt::CaseInsensitive))
        return QString();

    start += 7;
    QString id = fileName.mid(start, fileName.size() - start - 4).toLower();

    // 每一段都得是合法的十六进制，不然这个文件不是我们要的
    for (const QString& part : id.split('_')) {
        bool ok = false;
        part.toUInt(&ok, 16);
        if (!ok)
            return QString();
    }

    return id;
}

QString EmojiManager::idToText(const QString& id)
{
    // Noto 的文件名把 FE0F 都省掉了，这里按规则补回去，
    // 生成的是标准写法，发到别的软件里也能正确显示
    QString text;

    for (const QString& part : id.split('_')) {
        char32_t cp = part.toUInt(nullptr, 16);
        text += QString::fromUcs4(&cp, 1);

        bool needVariation = (cp < 0x10000 && cp != kZwj) || kTextStyleWide.contains(cp);
        if (needVariation)
            text += QChar(kVariation);
    }

    return text;
}

QString EmojiManager::stripVariation(const QString& text)
{
    QString out = text;
    out.remove(QChar(kVariation));
    return out;
}