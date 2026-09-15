#ifndef REQUESTWINDOW_H
#define REQUESTWINDOW_H

#include "../../NoFrame.h"
#include "../../Protocol.h"
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QJsonArray>

// 好友申请列表弹窗，跟 AddFriendWindow 一个风格，只是没有搜索框
class RequestWindow : public NoFrame
{
    Q_OBJECT

public:
    explicit RequestWindow(QWidget* parent = nullptr);
    ~RequestWindow();

    // --- 给逻辑层调用的接口 ---
    void setRequests(const QJsonArray& requests);   // 整体重建列表
    void addRequest(const QString& username, const QString& nickname,
        const QString& avatar);                     // 来了新的一条
    void removeRequest(const QString& username);    // 处理完了，抽掉这一行
    void setItemBusy(const QString& username, bool busy);

signals:
    void sendHandleRequest(const QString& username, int action);

private:
    void initUI();
    void appendItem(const QString& username, const QString& nickname,
        const QString& avatar);
    void refreshEmptyHint();
    int  indexOf(const QString& username) const;

    static QString avatarPath(const QString& avatar);

private:
    QWidget* content;
    QVBoxLayout* mainLayout;

    QLabel* lbTitle;
    QListWidget* listWidget;
    QLabel* lbEmpty;
};

#endif // REQUESTWINDOW_H