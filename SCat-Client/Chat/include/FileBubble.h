#ifndef FILEBUBBLE_H
#define FILEBUBBLE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "ChatMessage.h"

// 文件消息的气泡。它只是个显示器 —— 传输状态在 FileTransfer 里，
// 这里只负责把状态画出来，以及把用户的点击抛上去。
// 尺寸固定，不随状态变高，省掉一堆重算 QListWidgetItem 高度的麻烦
class FileBubble : public QWidget
{
    Q_OBJECT

public:
    explicit FileBubble(const ChatMessage& msg, QWidget* parent = nullptr);

    QString msgid() const { return id; }

    void setProgress(qint64 done, qint64 total);
    void setState(int newState);
    void setFilePath(const QString& p) { path = p; }

signals:
    void cancelClicked(const QString& msgid);
    void retryClicked(const QString& msgid, const QString& to, const QString& filePath);
    void openClicked(const QString& msgid, const QString& filePath);
    void downloadClicked(const QString& msgid);

private slots:
    void onActionClicked();

private:
    void initUI();
    void refresh();          // 按当前 state 决定文字、按钮、进度条显隐

private:
    QString id;
    QString peer;            // 重试时要知道发给谁
    QString path;
    QString name;
    qint64  size;
    bool    self;
    int     state;

    QString progressText;    // "45%"，只在传输中有值

    QWidget* card;
    QLabel* lbIcon;
    QLabel* lbName;
    QLabel* lbInfo;
    QProgressBar* bar;
    QPushButton* btnAction;
};

#endif // FILEBUBBLE_H