#include "../include/EmojiLabel.h"
#include "../include/EmojiManager.h"
#include <QPainter>
#include <QFontMetrics>

static const int kEmojiSize = 16;      // 配 13px 的灰色小字，比聊天区的 20px 小一号
static const int kEmojiPadding = 1;    // 左右各留 1px，跟气泡里一样不贴字
static const QString kEllipsis = QStringLiteral("…");

EmojiLabel::EmojiLabel(const QString& text, QWidget* parent) : QWidget(parent)
{
    // 横向能被压窄（压窄了就省略），纵向固定一行高
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setText(text);
}

void EmojiLabel::setText(const QString& text)
{
    content = text;
    parse();

    // 宽度变了，通知布局重新排一下
    updateGeometry();
    update();
}

void EmojiLabel::parse()
{
    segments.clear();

    // 只显示一行，换行一律当空格
    QString text = content;
    text.replace('\n', ' ');

    EmojiManager& em = EmojiManager::GetInstance();
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
            segments.append({ run, QString() });
            run.clear();
        }

        segments.append({ QString(), id });
        i += len;
    }

    if (!run.isEmpty())
        segments.append({ run, QString() });
}

int EmojiLabel::segmentWidth(const Segment& seg) const
{
    if (!seg.emojiId.isEmpty())
        return kEmojiSize + kEmojiPadding * 2;

    return fontMetrics().horizontalAdvance(seg.text);
}

QSize EmojiLabel::sizeHint() const
{
    int width = 0;
    for (const Segment& seg : segments)
        width += segmentWidth(seg);

    return QSize(width, qMax(fontMetrics().height(), kEmojiSize));
}

QSize EmojiLabel::minimumSizeHint() const
{
    return QSize(0, sizeHint().height());
}

void EmojiLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setPen(palette().color(QPalette::WindowText));

    QFontMetrics fm = fontMetrics();
    EmojiManager& em = EmojiManager::GetInstance();
    qreal dpr = devicePixelRatioF();

    // 文字和表情都在这一行里垂直居中
    int baseline = (height() - fm.height()) / 2 + fm.ascent();
    int emojiTop = (height() - kEmojiSize) / 2;

    // 整行放得下就全画；放不下的话，要给末尾的"…"留出位置
    int avail = width();
    bool elide = sizeHint().width() > avail;
    int limit = elide ? avail - fm.horizontalAdvance(kEllipsis) : avail;

    int x = 0;

    for (const Segment& seg : segments) {
        int w = segmentWidth(seg);

        if (x + w <= limit) {
            if (seg.emojiId.isEmpty())
                painter.drawText(x, baseline, seg.text);
            else
                painter.drawPixmap(QPointF(x, emojiTop),
                    em.inlinePixmap(seg.emojiId, kEmojiSize, kEmojiPadding, dpr));

            x += w;
            continue;
        }

        // 这一段放不下了。文字就一个字一个字往回减到放得下为止，
        // 表情没法切半个，直接整个不画
        if (seg.emojiId.isEmpty()) {
            QString part = seg.text;

            while (!part.isEmpty() && x + fm.horizontalAdvance(part) > limit) {
                part.chop(1);

                // 别把一个代理对切成两半，剩下半个字符会画成乱码
                if (!part.isEmpty() && part.back().isHighSurrogate())
                    part.chop(1);
            }

            painter.drawText(x, baseline, part);
            x += fm.horizontalAdvance(part);
        }

        painter.drawText(x, baseline, kEllipsis);
        break;
    }
}