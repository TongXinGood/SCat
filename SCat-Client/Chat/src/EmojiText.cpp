#include "../include/EmojiText.h"
#include "../include/EmojiManager.h"
#include <QTextBlock>

namespace EmojiText
{

    static const char* kScheme = "emoji";     // 图片名的前缀，"emoji:1f600"

    QTextImageFormat imageFormat(const QString& id, int size, int padding)
    {
        // 宽高写死成逻辑像素，高分屏上 Qt 会用 2 倍大的图去填，所以不糊
        QTextImageFormat fmt;
        fmt.setName(QString("%1:%2").arg(kScheme, id));
        fmt.setWidth(size + padding * 2);
        fmt.setHeight(size);

        // 底边贴着行底，表情正好跟中文字居中，行高也不会被撑大。
        // AlignMiddle 会偏下，AlignBaseline 会把整行顶高
        fmt.setVerticalAlignment(QTextCharFormat::AlignBottom);
        return fmt;
    }

    void insert(QTextCursor& cursor, const QString& text, int size, int padding)
    {
        EmojiManager& em = EmojiManager::GetInstance();

        // 普通文字用一个干净的格式插。光标紧挨着表情时，它的"当前格式"是图片格式，
        // 直接沿用的话插进来的字也会被当成图片画
        QTextCharFormat plain;
        QString run;
        int i = 0;

        while (i < text.size()) {
            QString id;
            int len = em.matchAt(text, i, id);

            if (len == 0) {
                run += text.at(i++);
                continue;
            }

            if (!run.isEmpty()) {
                cursor.insertText(run, plain);
                run.clear();
            }

            cursor.insertImage(imageFormat(id, size, padding));
            i += len;
        }

        if (!run.isEmpty())
            cursor.insertText(run, plain);
    }

    QString toPlainText(const QTextDocument* doc, int from, int to)
    {
        EmojiManager& em = EmojiManager::GetInstance();
        QString prefix = QString("%1:").arg(kScheme);
        QString out;
        bool first = true;

        for (QTextBlock block = doc->findBlock(from);
            block.isValid() && block.position() <= to; block = block.next()) {

            // 两段之间是回车（Shift+回车换的行）
            if (!first)
                out += '\n';
            first = false;

            for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
                QTextFragment frag = it.fragment();
                int start = qMax(frag.position(), from);
                int end = qMin(frag.position() + frag.length(), to);
                if (start >= end)
                    continue;

                QTextCharFormat fmt = frag.charFormat();

                if (fmt.isImageFormat()) {
                    QString name = fmt.toImageFormat().name();
                    if (!name.startsWith(prefix))
                        continue;

                    // 连着两个一样的表情，Qt 会把它们并成一个片段，有几格就输出几次
                    QString emoji = em.textOf(name.mid(prefix.size()));
                    for (int i = start; i < end; ++i)
                        out += emoji;
                }
                else {
                    out += frag.text().mid(start - frag.position(), end - start);
                }
            }
        }

        // 跟 toPlainText 的处理保持一致
        out.replace(QChar::LineSeparator, '\n');
        out.replace(QChar::Nbsp, ' ');
        return out;
    }

    QVariant loadResource(const QUrl& name, int size, int padding, qreal dpr)
    {
        if (name.scheme() != kScheme)
            return QVariant();

        QPixmap pix = EmojiManager::GetInstance().inlinePixmap(name.path(), size, padding, dpr);
        if (pix.isNull())
            return QVariant();

        return pix;
    }
}