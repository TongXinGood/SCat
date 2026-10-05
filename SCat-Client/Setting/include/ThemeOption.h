#ifndef THEMEOPTION_H
#define THEMEOPTION_H

#include <QAbstractButton>

// 设置页"外观"卡片里的一个选项：上面一张迷你界面缩略图，下面一行字。
// 三个放一排、互斥，选中的那个描紫边、右上角打勾
class ThemeOption : public QAbstractButton
{
    Q_OBJECT

public:
    enum Kind { Light, Dark, System };

    ThemeOption(Kind kind, const QString& text, QWidget* parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void drawMiniWindow(QPainter& p, const QRectF& rect, bool dark) const;

private:
    Kind kind;
    bool hovered;
};

#endif // THEMEOPTION_H