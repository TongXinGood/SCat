#ifndef EMOJIPICKER_H
#define EMOJIPICKER_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include "EmojiManager.h"

// 点表情按钮弹出来的面板，跟 QQ、微信一样：点外面自动关，点一个表情也关。
// 它只管"选了哪个"，选中的表情怎么插进输入框是 ChatWindow 的事。
// 整个窗口建一次反复用，第二次打开时还停在上次滚到的位置。
// 最上面一排是"最近使用"，按账号分开记，每次打开面板时重新读
class EmojiPicker : public QWidget
{
    Q_OBJECT

public:
    explicit EmojiPicker(QWidget* parent = nullptr);

    // 弹在 anchor（表情按钮）正上方，右边跟它对齐
    void popup(QWidget* anchor);

signals:
    void emojiSelected(const QString& text);   // 选中的表情，Unicode 文字

private:
    void initUi();
    QWidget* createSection(const QString& title, const QList<EmojiItem>& items);

    void refreshRecent();                 // 按存下来的记录重建"最近使用"那一段
    QStringList loadRecent() const;
    void addRecent(const QString& id);    // 选了一个表情，挪到最近使用的最前面

private:
    QWidget* card;          // 白色圆角卡片。外面一圈透明边距是留给阴影的
    QScrollArea* scroll;
    QVBoxLayout* contentLayout;
    QWidget* recentSection;  // 没用过任何表情时为空，不显示这一段
};

#endif // EMOJIPICKER_H