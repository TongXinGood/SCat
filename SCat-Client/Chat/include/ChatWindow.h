#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit> 
#include <QListWidget>
#include <QKeyEvent> 
#include "ChatMessage.h"

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget* parent = nullptr);
    ~ChatWindow();

    // --- 外部接口 ---
    void setChatInfo(const QString& name, const QString& status, const QString& avatarPath);
    void addMessage(const QString& msg, bool isSelf);
    void addFileMessage(const QString& fileName, const QString& fileSize, bool isSelf);
    void setHistory(const QList<ChatMessage>& list);   // 切换好友时整段重绘
    void appendMessage(const ChatMessage& msg);        // 来一条新的，追加一条

signals:
    void sendTextMsg(const QString& msg);
    void sendFileClicked();
    void sendMoodClicked(); // 新增：点击表情按钮的信号

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onReturnPressed();
    void onFileBtnClicked();
    void onMoodBtnClicked();

private:
    void initUi();
    void initHeader();
    void initMsgList();
    void initInput();

    QWidget* createBubbleWidget(const QString& text, bool isSelf);
    QWidget* createFileBubbleWidget(const QString& fileName, const QString& fileSize, bool isSelf);

private:
    // --- UI 控件 ---
    QWidget* headerWidget;
    QLabel* lbAvatar;
    QLabel* lbName;
    QLabel* lbStatus;
    QPushButton* btnMore;

    QListWidget* msgList;

    QWidget* inputContainer;
    QPushButton* btnmood; // 改为 QPushButton
    QTextEdit* msgEdit;
    QPushButton* btnFile;
};

#endif // CHATWINDOW_H