#include "../include/ChatInputEdit.h"
#include "../include/EmojiManager.h"
#include "../include/EmojiText.h"
#include "../../Other/include/ImageUtils.h"
#include "../../Other/include/Theme.h"
#include <QMimeData>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QHash>
#include <QUrl>
#include <QDebug>
#include <QTextBlock>
#include <QTextDocument>

static const int kEmojiSize = 20;              // 框里表情的大小，比 14px 的字略大一点
static const int kEmojiPadding = 1;            // 表情左右各留的空，不然紧贴着前后的字

static const char* kMenuStyle = R"(
    QMenu {
        background-color: #FFFFFF;
        border: 1px solid #ECE9F2;
        border-radius: 10px;
        padding: 6px;
    }
    QMenu::item {
        background-color: transparent;
        color: #1A1A1A;
        font-size: 13px;
        padding: 8px 32px 8px 16px;
        margin: 1px 2px;
        border-radius: 6px;
    }
    QMenu::item:selected {
        background-color: #F4F1FA;
        color: #20202E;
    }
    QMenu::item:disabled {
        color: #C0C0C0;
    }
    QMenu::separator {
        height: 1px;
        background-color: #F0F0F0;
        margin: 5px 8px;
    }
)";


ChatInputEdit::ChatInputEdit(QWidget* parent) : QTextEdit(parent), convertPending(false)
{
    // 只收纯文本。开着富文本的话，从网页复制过来会把字体、颜色一起带进来，
    // 发出去的却只有纯文字，所见非所得
    setAcceptRichText(false);

    // 只在"新增了一步编辑"时扫，撤销、重做不会触发这个信号。
    // 要是撤销也扫，撤回来的表情文字马上又被换成图片，永远撤不掉
    connect(document(), &QTextDocument::undoCommandAdded, this, &ChatInputEdit::scheduleConvert);
}

void ChatInputEdit::insertEmoji(const QString& text)
{
    QTextCursor cursor = textCursor();

    // 包成一个编辑块，Ctrl+Z 一下就整个撤掉
    cursor.beginEditBlock();
    EmojiText::insert(cursor, text, kEmojiSize, kEmojiPadding);
    cursor.endEditBlock();

    setTextCursor(cursor);
    ensureCursorVisible();
}

QString ChatInputEdit::messageText() const
{
    // characterCount 把文档末尾那个看不见的段落符也算进去了，要减掉
    return EmojiText::toPlainText(document(), 0, document()->characterCount() - 1);
}

void ChatInputEdit::scheduleConvert()
{
    // 不能在信号里直接改文档，那时 Qt 自己还在改的半路上。
    // 排到事件循环里稍后再扫，连续打字触发多次也只扫一次
    if (convertPending)
        return;

    convertPending = true;
    QMetaObject::invokeMethod(this, &ChatInputEdit::convertEmoji, Qt::QueuedConnection);
}

void ChatInputEdit::convertEmoji()
{
    convertPending = false;

    EmojiManager& em = EmojiManager::GetInstance();
    QTextDocument* doc = document();

    struct Hit
    {
        int pos;
        int len;
        QString id;
    };
    QList<Hit> hits;

    // 先把所有表情的位置找出来，再统一替换
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid() || frag.charFormat().isImageFormat())
                continue;

            QString text = frag.text();
            int i = 0;

            while (i < text.size()) {
                QString id;
                int len = em.matchAt(text, i, id);

                if (len > 0) {
                    hits.append({ frag.position() + i, len, id });
                    i += len;
                }
                else {
                    ++i;
                }
            }
        }
    }

    // 平时打字走到这里都是空手而归，只有 Win+. 这类入口才会真的换
    if (hits.isEmpty())
        return;

    QTextCursor cursor(doc);
    cursor.beginEditBlock();

    // 从后往前换：一个表情两三个字符换成一张图只占一格，
    // 从前往后换的话，后面记下的位置就全错了
    for (int k = hits.size() - 1; k >= 0; --k) {
        const Hit& hit = hits.at(k);

        cursor.setPosition(hit.pos);
        cursor.setPosition(hit.pos + hit.len, QTextCursor::KeepAnchor);
        cursor.insertImage(EmojiText::imageFormat(hit.id, kEmojiSize, kEmojiPadding));
    }

    cursor.endEditBlock();
}

QVariant ChatInputEdit::loadResource(int type, const QUrl& name)
{
    if (type == QTextDocument::ImageResource) {
        QVariant emoji = EmojiText::loadResource(name, kEmojiSize, kEmojiPadding, devicePixelRatioF());
        if (emoji.isValid())
            return emoji;
    }

    return QTextEdit::loadResource(type, name);
}

QMimeData* ChatInputEdit::createMimeDataFromSelection() const
{
    QTextCursor cursor = textCursor();

    QMimeData* mime = new QMimeData();
    mime->setText(EmojiText::toPlainText(document(), cursor.selectionStart(), cursor.selectionEnd()));
    return mime;
}

void ChatInputEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Shift+回车 = 换行，落到基类照常处理
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            emit sendPressed();
            return;
        }
    }

    QTextEdit::keyPressEvent(event);
}

bool ChatInputEdit::hasImage(const QMimeData* source) const
{
    if (!source)
        return false;

    if (source->hasImage())
        return true;

    if (source->hasUrls()) {
        for (const QUrl& url : source->urls()) {
            if (url.isLocalFile() && ImageUtils::isImageFile(url.toLocalFile()))
                return true;
        }
    }

    return false;
}

QImage ChatInputEdit::imageFrom(const QMimeData* source) const
{
    if (!source)
        return QImage();

    // 情况一：截图工具、或者在别的软件里"复制图片"，
    // 剪贴板里直接就是图片数据，没有文件
    if (source->hasImage())
        return qvariant_cast<QImage>(source->imageData());

    // 情况二：在资源管理器里复制了图片文件，剪贴板里是路径。
    // 选中一堆也只取第一张，一次发一张更符合直觉
    if (source->hasUrls()) {
        for (const QUrl& url : source->urls()) {
            if (!url.isLocalFile())
                continue;

            QString path = url.toLocalFile();
            if (!ImageUtils::isImageFile(path))
                continue;

            QString error;
            QImage img = ImageUtils::loadFile(path, error);

            if (img.isNull())
                qDebug() << "paste image failed:" << path << error;

            return img;
        }
    }

    return QImage();
}

bool ChatInputEdit::canInsertFromMimeData(const QMimeData* source) const
{
    // setAcceptRichText(false) 之后基类只认文本。剪贴板里是一张截图时
    // （只有图片数据、没有文本）它会直接判定"不能粘贴"，
    // insertFromMimeData 根本不会被调到。这里先放行图片
    if (hasImage(source))
        return true;

    return QTextEdit::canInsertFromMimeData(source);
}

void ChatInputEdit::insertFromMimeData(const QMimeData* source)
{
    QImage img = imageFrom(source);

    if (!img.isNull()) {
        emit imagePasted(img);
        return;      // 不调基类，图片就不会被塞进文本框里
    }

    // 文字自己插，顺手把里面的表情换成图片。交给基类的话，
    // 光标挨着表情时粘进来的字会沿用图片格式，整段画成图
    if (source && source->hasText()) {
        insertEmoji(source->text());
        return;
    }

    QTextEdit::insertFromMimeData(source);
}

void ChatInputEdit::contextMenuEvent(QContextMenuEvent* event)
{
    // 让基类先把 撤销/剪切/复制/粘贴 这些标准动作建好，我们只负责换皮。
    // 自己从零拼一遍既啰嗦，又容易漏动作、漏禁用状态
    QMenu* menu = createStandardContextMenu();
    if (!menu)
        return;

    // 圆角必须配透明背景。QMenu 是个不透明的顶层窗口，
    // 圆角切掉的四个角会露出窗口底色（深色模式下就是黑的），
    // 看着像四个黑三角。顺手关掉系统阴影，不然阴影还是方的
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setWindowFlags(menu->windowFlags()
        | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    menu->setStyleSheet(Theme::css(kMenuStyle));

    // Qt 给每个标准动作都起了 objectName，按它翻译最稳 ——
    // 按文字匹配会被 Qt 的翻译影响，按顺序匹配会被动作的增减打乱
    static const QHash<QString, QString> names = {
        { QStringLiteral("edit-undo"),   QStringLiteral("撤销") },
        { QStringLiteral("edit-redo"),   QStringLiteral("重做") },
        { QStringLiteral("edit-cut"),    QStringLiteral("剪切") },
        { QStringLiteral("edit-copy"),   QStringLiteral("复制") },
        { QStringLiteral("edit-paste"),  QStringLiteral("粘贴") },
        { QStringLiteral("edit-delete"), QStringLiteral("删除") },
        { QStringLiteral("select-all"),  QStringLiteral("全选") },
    };

    for (QAction* action : menu->actions()) {
        QString name = names.value(action->objectName());
        if (name.isEmpty())
            continue;

        // 原文长这样："&Undo\tCtrl+Z"，\t 后面是快捷键提示。
        // 只换前半截，右边的 Ctrl+Z 保留
        int tab = action->text().indexOf('\t');
        if (tab >= 0)
            name += action->text().mid(tab);

        action->setText(name);
    }

    menu->exec(event->globalPos());

    // createStandardContextMenu 返回的菜单没有父对象，归调用方管
    delete menu;
}