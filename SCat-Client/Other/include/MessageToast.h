#ifndef MESSAGETOAST_H
#define MESSAGETOAST_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>

class QLabel;
class QPushButton;
class EmojiLabel;

// 右下角的新消息弹窗，代替系统托盘气泡。
// 系统气泡的头像被 Windows 固定成二十来像素的小图标，表情也是系统样式，
// 所以自己画一个：圆形大头像、Noto 表情、跟软件一套的配色。
// 只管自己长什么样、什么时候消失；几个弹窗怎么叠放是 Notification 的事
class MessageToast : public QWidget
{
    Q_OBJECT

public:
    explicit MessageToast(QWidget* parent = nullptr);

    // 填内容。同一个人连着发，Notification 会带着新的条数再调一次
    void setMessage(const QString& peer, const QString& name,
        const QPixmap& avatar, const QString& text, int count);

    QString peer() const { return peerId; }
    int count() const { return msgCount; }

    void popup(const QPoint& pos);     // 从 pos 下方一点滑上来、淡入
    void moveTo(const QPoint& pos);    // 别的弹窗关了，平滑挪到新位置
    void dismiss();                    // 淡出，结束后发 closed 并自己删掉
    void restartTimer();               // 又来了一条，重新计时

signals:
    void clicked(const QString& peer);
    void closed(MessageToast* self);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initUi();

private:
    QString peerId;
    int msgCount;
    bool dismissing;      // 已经在淡出了，别重复处理

    QWidget* card;
    QLabel* lbAvatar;
    QLabel* lbBadge;      // 头像右上角的红点，连发几条就显示几
    QLabel* lbName;
    QLabel* lbTime;
    QPushButton* btnClose;
    EmojiLabel* lbText;

    QTimer lifeTimer;     // 到点自动消失。鼠标停在上面时暂停
};

#endif // MESSAGETOAST_H