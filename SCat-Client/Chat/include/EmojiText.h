#ifndef EMOJITEXT_H
#define EMOJITEXT_H

#include <QString>
#include <QUrl>
#include <QVariant>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextImageFormat>

// 带表情的文字跟 QTextDocument 之间的互相转换。输入框和文字气泡共用，
// 想改表情在文字里怎么排、复制出来是什么样，只要动这一个文件。
// 表情在文档里是一张名叫 "emoji:1f600" 的图片，图由各控件的 loadResource 现取
namespace EmojiText
{
    // 一个表情对应的图片格式。size 是表情大小，padding 是左右各留的空
    QTextImageFormat imageFormat(const QString& id, int size, int padding);

    // 在 cursor 处插入 text，里面的表情变成图片，其余的字照常插
    void insert(QTextCursor& cursor, const QString& text, int size, int padding);

    // 把文档 [from, to) 这一段还原成文字，表情图片换回 Unicode。
    // 发送、复制都靠它，toPlainText 拿到的表情全是占位符
    QString toPlainText(const QTextDocument* doc, int from, int to);

    // 控件的 loadResource 先问这里。是表情就返回缩好的图，不是返回空 QVariant
    QVariant loadResource(const QUrl& name, int size, int padding, qreal dpr);
}

#endif // EMOJITEXT_H