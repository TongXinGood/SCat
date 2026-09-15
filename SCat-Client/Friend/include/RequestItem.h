#ifndef REQUESTITEM_H
#define REQUESTITEM_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

// 申请列表里的一行：头像 + 昵称/账号 + 同意/拒绝两个按钮
class RequestItem : public QWidget
{
    Q_OBJECT

public:
    RequestItem(const QString& username, const QString& nickname,
        const QString& avatarPath, QWidget* parent = nullptr);

    QString username() const { return user; }

    // 请求发出去了还没回来，先把两个按钮锁住，防止连点
    void setBusy(bool busy);

signals:
    void sendAccept(const QString& username);
    void sendReject(const QString& username);

private:
    void initUI(const QString& nickname, const QString& avatarPath);

private:
    QString user;

    QLabel* lbAvatar;
    QLabel* lbNickname;
    QLabel* lbUsername;
    QPushButton* btnAccept;
    QPushButton* btnReject;
};

#endif // REQUESTITEM_H