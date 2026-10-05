#include "../include/TextBubble.h"
#include "../include/EmojiText.h"
#include "../../Other/include/Theme.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QClipboard>
#include <QGuiApplication>
#include <QContextMenuEvent>
#include <QWheelEvent>
#include <QtMath>

static const int kEmojiSize = 20;          // 跟输入框里一样大，发出去前后看着一致
static const int kEmojiPadding = 1;
static const int kMaxTextWidth = 418;      // 原来 QLabel 最大宽 450，减掉左右各 16 的内边距

BubbleTextView::BubbleTextView(QWidget* parent) : QTextEdit(parent)
{
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    // 外观全交给外面的气泡，这里只是一块透明的字
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);
    document()->setDocumentMargin(0);

    // 一长串没有空格的链接、数字也能折行，不会把气泡撑出屏幕
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
}

void BubbleTextView::setMessage(const QString& text)
{
    content = text;

    // 样式表里的字体、颜色要等 polish 之后才生效，先让它生效再量
    ensurePolished();

    QTextDocument* doc = document();
    doc->clear();
    doc->setDefaultFont(font());

    QTextCursor cursor(doc);
    EmojiText::insert(cursor, text, kEmojiSize, kEmojiPadding);

    // 先不限宽量出一行排下要多宽，超过上限再按上限折行。
    // 多加 1 像素，不然小数宽度向下取整后最后一个字会被挤到下一行
    doc->setTextWidth(-1);
    int width = qMin(qCeil(doc->idealWidth()) + 1, kMaxTextWidth);

    doc->setTextWidth(width);
    int height = qCeil(doc->size().height());

    setFixedSize(width, height);
}

QVariant BubbleTextView::loadResource(int type, const QUrl& name)
{
    if (type == QTextDocument::ImageResource) {
        QVariant emoji = EmojiText::loadResource(name, kEmojiSize, kEmojiPadding, devicePixelRatioF());
        if (emoji.isValid())
            return emoji;
    }

    return QTextEdit::loadResource(type, name);
}

QMimeData* BubbleTextView::createMimeDataFromSelection() const
{
    QTextCursor cursor = textCursor();

    QMimeData* mime = new QMimeData();
    mime->setText(EmojiText::toPlainText(document(), cursor.selectionStart(), cursor.selectionEnd()));
    return mime;
}

void BubbleTextView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    // 样式用 main.cpp 里全局的那套，这里只补上圆角需要的透明背景，
    // 原因见 ChatInputEdit::contextMenuEvent
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setWindowFlags(menu.windowFlags()
        | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    QAction* actCopy = menu.addAction("复制");

    if (menu.exec(event->globalPos()) != actCopy)
        return;

    // 选中了一段就复制那一段，什么都没选就复制整条，跟微信一样
    QTextCursor cursor = textCursor();
    QString text = cursor.hasSelection()
        ? EmojiText::toPlainText(document(), cursor.selectionStart(), cursor.selectionEnd())
        : content;

    QGuiApplication::clipboard()->setText(text);
}

void BubbleTextView::wheelEvent(QWheelEvent* event)
{
    // 自己没有滚动条，滚轮事件不处理，让它冒泡给外面的 QListWidget
    event->ignore();
}

TextBubble::TextBubble(const QString& text, bool isSelf, QWidget* parent)
    : QWidget(parent)
{
    initUi(text, isSelf);
}

void TextBubble::initUi(const QString& text, bool isSelf)
{
    setStyleSheet("background-color: transparent;");

    QHBoxLayout* outer = new QHBoxLayout(this);
    outer->setContentsMargins(10, 10, 10, 10);

    // 圆角底色画在这一层上。QTextEdit 自己带的圆角会被里面的 viewport 盖住，
    // 所以分成两层：外面画底色，里面只放透明的字
    QFrame* card = new QFrame(this);
    card->setObjectName("BubbleCard");

    // 大小只由里面的字决定。不锁住的话，自己发的那边会跟左侧的弹簧平分空位，
    // 气泡被拉得老长
    card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QHBoxLayout* cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(16, 12, 16, 12);

    BubbleTextView* view = new BubbleTextView(card);
    QFont font = QGuiApplication::font();
    font.setPointSize(10);
    view->setFont(font);

    // 配色跟原来的 QLabel 气泡一致，另外配了选中文字时的底色：
    // 深色气泡上默认的蓝色选区太扎眼，换成同色系
    if (isSelf) {
        card->setStyleSheet(Theme::css("QFrame#BubbleCard { background-color: #20202E; border-radius: 12px; }"));
        view->setStyleSheet(Theme::css("QTextEdit { background: transparent; color: #FFFFFF;"
            " selection-background-color: #4A4A66; selection-color: #FFFFFF; }"));
    }
    else {
        card->setStyleSheet(Theme::css("QFrame#BubbleCard { background-color: #FFFFFF; border-radius: 12px; }"));
        view->setStyleSheet(Theme::css("QTextEdit { background: transparent; color: #000000;"
            " selection-background-color: #DCD4F0; selection-color: #000000; }"));
    }

    // 字体和样式都设好了再填内容，量出来的大小才准
    view->setMessage(text);
    cardLayout->addWidget(view);

    if (isSelf) {
        outer->addStretch();
        outer->addWidget(card);
    }
    else {
        outer->addWidget(card);
        outer->addStretch();
    }
}