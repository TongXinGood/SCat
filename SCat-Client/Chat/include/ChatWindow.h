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
#include <QImage>
#include <QHash>
#include "ChatMessage.h"
#include "ChatInputEdit.h"
#include "FileBubble.h"

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget* parent = nullptr);
    ~ChatWindow();

    // --- 外部接口 ---
    void setChatInfo(const QString& name, const QString& status, const QPixmap& avatar);
    void setAvatar(const QPixmap& avatar);
    void setHistory(const QList<ChatMessage>& list);   // 切换好友时整段重绘
    void appendMessage(const ChatMessage& msg);        // 来一条新的，追加一条
    void updateFileProgress(const QString& msgid, qint64 done, qint64 total);
    void updateFileState(const QString& msgid, int state,const QString& filePath = QString());

signals:
    void sendTextMsg(const QString& msg);
    void sendFileClicked();
    void sendImage(const QImage& image);
    void sendMoodClicked(); // 新增：点击表情按钮的信号
    void fileCancelClicked(const QString& msgid);
    void fileRetryClicked(const QString& msgid, const QString& to,const QString& filePath);
    void fileOpenClicked(const QString& msgid, const QString& filePath);
    void fileDownloadClicked(const QString& msgid);
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
    void addBubble(const ChatMessage& msg);

    QWidget* createBubbleWidget(const QString& text, bool isSelf);
    QWidget* createImageBubbleWidget(const ChatMessage& msg);
    QWidget* createFileBubbleWidget(const ChatMessage& msg);

private:
    // --- UI 控件 ---
    QWidget* headerWidget;
    QLabel* lbAvatar;
    QLabel* lbName;
    QLabel* lbStatus;
    QPushButton* btnMore;

    QListWidget* msgList;

    QWidget* inputContainer;
    QHash<QString, FileBubble*> fileBubbles;
    QPushButton* btnmood; // 改为 QPushButton
    ChatInputEdit* msgEdit;
    QPushButton* btnFile;
};

#endif // CHATWINDOW_H