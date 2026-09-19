#ifndef CHATINPUTEDIT_H
#define CHATINPUTEDIT_H

#include <QTextEdit>
#include <QImage>

// 聊天输入框。在 QTextEdit 基础上做两件事：
//   1. 回车发送、Shift+回车换行（原来是 ChatWindow 装 eventFilter 干的，搬进来更内聚）
//   2. 粘贴进来的图片不往文本里插，转成信号抛给上层去发送
class ChatInputEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget* parent = nullptr);

signals:
    void sendPressed();                        // 按了回车（没按 Shift）
    void imagePasted(const QImage& image);     // 粘贴进来一张图

protected:
    void keyPressEvent(QKeyEvent* event) override;

    // 所有粘贴入口都会走这里：Ctrl+V、右键菜单、中键粘贴。
    // 只拦按键的话右键粘贴就漏了
    void insertFromMimeData(const QMimeData* source) override;

    // 基类会先问一句"这东西能粘吗"，说不能就根本不会调上面那个。
    // 必须一起重写，否则截图永远粘不进来
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    bool   hasImage(const QMimeData* source) const;   // 只判断有没有，不真的去读
    QImage imageFrom(const QMimeData* source) const;  // 真正把图取出来
};

#endif // CHATINPUTEDIT_H