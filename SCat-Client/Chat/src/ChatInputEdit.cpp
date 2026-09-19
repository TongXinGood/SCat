#include "../include/ChatInputEdit.h"
#include "../../Other/include/ImageUtils.h"
#include <QMimeData>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QHash>
#include <QUrl>
#include <QDebug>

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


ChatInputEdit::ChatInputEdit(QWidget* parent) : QTextEdit(parent)
{
    // 只收纯文本。开着富文本的话，从网页复制过来会把字体、颜色一起带进来，
    // 发出去的却只有纯文字，所见非所得
    setAcceptRichText(false);
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

    menu->setStyleSheet(kMenuStyle);

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