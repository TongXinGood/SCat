#ifndef EMOJIMANAGER_H
#define EMOJIMANAGER_H

#include <QString>
#include <QList>
#include <QHash>
#include <QPixmap>

// 一个表情
struct EmojiItem
{
    QString id;      // 文件名里的编码部分，如 "1f600"、"1f62e_200d_1f4a8"。
    // 程序内部靠它认表情，不会出现在消息里
    QString text;    // 对应的 Unicode 文字。发出去、存进库里的就是它
    QString path;    // 资源路径，如 ":/Resource/emoji/smileys/emoji_u1f600.png"
};

// 一个分类，对应 Resource/emoji 下的一个子文件夹
struct EmojiCategory
{
    QString key;              // 文件夹名，如 "smileys"
    QString name;             // 显示名，如 "黄脸"
    QList<EmojiItem> items;
};

// 全局单例：表情的识别和图片缓存。
// 消息里传的始终是 Unicode 文字，这里只负责"哪段文字是表情、该画哪张图"。
// 第一次 GetInstance 时扫描资源目录，往 qrc 里加图不用改这里的代码
class EmojiManager
{
public:
    static EmojiManager& GetInstance();

    const QList<EmojiCategory>& categories() const { return cats; }
    int count() const { return itemById.size(); }

    bool contains(const QString& id) const { return itemById.contains(id); }
    QString textOf(const QString& id) const;

    // 取缩放好的图。size 是逻辑像素，dpr 传控件的 devicePixelRatioF()，
    // 高分屏上才不会糊。缩过一次就缓存起来，滚动聊天记录时不用反复缩
    QPixmap pixmap(const QString& id, int size, qreal dpr = 1.0);

    QPixmap inlinePixmap(const QString& id, int size, int padding, qreal dpr = 1.0);

    // 看 text 从 pos 开始是不是一个表情。
    // 是：返回它占了几个 QChar（含末尾的 FE0F），id 带出是哪个；不是：返回 0
    int matchAt(const QString& text, int pos, QString& id) const;

private:
    EmojiManager();
    ~EmojiManager() = default;

    // 单例，禁止拷贝
    EmojiManager(const EmojiManager&) = delete;
    EmojiManager& operator=(const EmojiManager&) = delete;

    void load();
    void loadCategory(const QString& key, const QString& name);

    static QString parseId(const QString& fileName);
    static QString idToText(const QString& id);
    static QString stripVariation(const QString& text);

private:
    QList<EmojiCategory> cats;
    QHash<QString, EmojiItem> itemById;
    QHash<QString, QString> idByKey;     // 去掉 FE0F 的文字 -> id，匹配时查它
    int maxKeyLen;                       // 最长的那个 key 有几个 QChar，匹配时往后看这么远就够了
    QHash<QString, QPixmap> pixCache;    // "id@size@dpr" -> 缩好的图
};

#endif // EMOJIMANAGER_H