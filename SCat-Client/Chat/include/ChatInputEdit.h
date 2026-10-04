#ifndef CHATINPUTEDIT_H
#define CHATINPUTEDIT_H

#include <QTextEdit>
#include <QImage>
#include <QMimeData>

// 聊天输入框。在 QTextEdit 基础上做三件事：
//   1. 回车发送、Shift+回车换行（原来是 ChatWindow 装 eventFilter 干的，搬进来更内聚）
//   2. 粘贴进来的图片不往文本里插，转成信号抛给上层去发送
//   3. 表情显示成 Noto 的图片，取文字时再换回 Unicode
class ChatInputEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget* parent = nullptr);

    // 在光标处插入一段文字，里面的表情直接变成图片。表情面板选中后调它
    void insertEmoji(const QString& text);

    // 要发出去的文字。表情在框里是图片，toPlainText 拿到的是一堆占位符，
    // 发送时必须用这个
    QString messageText() const;

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

    // 文档要画 "emoji:xxx" 这张图时会来问，从 EmojiManager 取缩好的图给它
    QVariant loadResource(int type, const QUrl& name) override;

    // 复制、剪切、拖拽都走这里。把表情图片换回文字，粘到别处才不会丢
    QMimeData* createMimeDataFromSelection() const override;

private:
    // 兜底：Win+. 输入法之类没经过 insertEmoji 的表情，事后扫一遍换成图片
    void scheduleConvert();
    void convertEmoji();

    bool   hasImage(const QMimeData* source) const;   // 只判断有没有，不真的去读
    QImage imageFrom(const QMimeData* source) const;  // 真正把图取出来

private:
    bool convertPending;       // 已经排了一次扫描还没执行，别重复排
};

#endif // CHATINPUTEDIT_H