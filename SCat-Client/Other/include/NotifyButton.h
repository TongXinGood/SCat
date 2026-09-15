#ifndef NOTIFYBUTTON_H
#define NOTIFYBUTTON_H

#include <QPushButton>

// 圆形图标按钮，右上角可以带一个红色数字角标。
// 现在给好友申请的铃铛用，以后好友列表的未读消息数也能复用
class NotifyButton : public QPushButton
{
    Q_OBJECT

public:
    explicit NotifyButton(QWidget* parent = nullptr);

    void setCount(int count);            // 0 = 不显示角标
    int count() const { return badgeCount; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int badgeCount;
};

#endif // NOTIFYBUTTON_H