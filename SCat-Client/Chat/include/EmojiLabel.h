#ifndef EMOJILABEL_H
#define EMOJILABEL_H

#include <QWidget>
#include <QList>

// 单行、带表情的小字。左侧会话列表的"最后一条消息"用它：
// 表情画成 Noto 小图，太长了末尾显示"…"，换行压成空格。
// 字号、颜色跟 QLabel 一样走样式表的 font-size、color
class EmojiLabel : public QWidget
{
    Q_OBJECT

public:
    explicit EmojiLabel(const QString& text = QString(), QWidget* parent = nullptr);

    void setText(const QString& text);
    QString text() const { return content; }

    QSize sizeHint() const override;

    // 最小宽度给 0，窄了就省略，不会像 QLabel 那样把整个列表撑宽
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // 一段普通文字，或者一个表情（emojiId 非空）
    struct Segment
    {
        QString text;
        QString emojiId;
    };

    void parse();
    int segmentWidth(const Segment& seg) const;

private:
    QString content;
    QList<Segment> segments;
};

#endif // EMOJILABEL_H