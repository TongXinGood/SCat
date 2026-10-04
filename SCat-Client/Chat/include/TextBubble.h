#ifndef TEXTBUBBLE_H
#define TEXTBUBBLE_H

#include <QWidget>
#include <QTextEdit>

// 气泡里显示文字的那块。只读的 QTextEdit，比 QLabel 多三样本事：
//   1. 表情画成 Noto 的图片
//   2. 拖选复制时把表情图片换回文字，粘出去不丢
//   3. 右键"复制"，没选中就复制整条
class BubbleTextView : public QTextEdit
{
    Q_OBJECT

public:
    explicit BubbleTextView(QWidget* parent = nullptr);

    void setMessage(const QString& text);   // 设内容，顺带把自己的大小定好
    QString message() const { return content; }

protected:
    QVariant loadResource(int type, const QUrl& name) override;
    QMimeData* createMimeDataFromSelection() const override;
    void contextMenuEvent(QContextMenuEvent* event) override;

    // 鼠标在气泡上滚滚轮，要让外面的聊天列表滚，不能被这里吃掉
    void wheelEvent(QWheelEvent* event) override;

private:
    QString content;    // 原始文字，右键复制整条时直接用它
};

// 一条文字消息的气泡：圆角底色 + 里面的文字，自己发的靠右、别人发的靠左。
// 跟 FileBubble 一个思路，外层一个横向布局负责左右摆放
class TextBubble : public QWidget
{
    Q_OBJECT

public:
    explicit TextBubble(const QString& text, bool isSelf, QWidget* parent = nullptr);

private:
    void initUi(const QString& text, bool isSelf);
};

#endif // TEXTBUBBLE_H