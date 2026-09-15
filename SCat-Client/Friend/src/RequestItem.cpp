#include "../include/RequestItem.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>

RequestItem::RequestItem(const QString& username, const QString& nickname,
    const QString& avatarPath, QWidget* parent)
    : QWidget(parent), user(username)
{
    initUI(nickname, avatarPath);
}

void RequestItem::initUI(const QString& nickname, const QString& avatarPath)
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setObjectName("RequestItem");

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(12);

    lbAvatar = new QLabel(this);
    lbAvatar->setFixedSize(44, 44);
    lbAvatar->setScaledContents(true);
    lbAvatar->setPixmap(QPixmap(avatarPath));
    lbAvatar->setStyleSheet("border-radius: 22px; background-color: #EFEBFA;");

    QVBoxLayout* nameLayout = new QVBoxLayout();
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(3);

    lbNickname = new QLabel(nickname, this);
    lbNickname->setObjectName("ReqNickname");

    lbUsername = new QLabel(user, this);
    lbUsername->setObjectName("ReqUsername");

    nameLayout->addStretch();
    nameLayout->addWidget(lbNickname);
    nameLayout->addWidget(lbUsername);
    nameLayout->addStretch();

    btnAccept = new QPushButton("同意", this);
    btnAccept->setObjectName("BtnAccept");
    btnAccept->setFixedSize(56, 30);
    btnAccept->setCursor(Qt::PointingHandCursor);

    btnReject = new QPushButton("拒绝", this);
    btnReject->setObjectName("BtnReject");
    btnReject->setFixedSize(56, 30);
    btnReject->setCursor(Qt::PointingHandCursor);

    mainLayout->addWidget(lbAvatar);
    mainLayout->addLayout(nameLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(btnAccept);
    mainLayout->addWidget(btnReject);

    this->setStyleSheet(R"(
        #RequestItem { background-color: transparent; }

        #ReqNickname {
            font-size: 15px; font-weight: bold; color: #1A1A1A;
            background: transparent; border: none;
        }
        #ReqUsername {
            font-size: 12px; color: #999999;
            background: transparent; border: none;
        }

        #BtnAccept {
            background-color: #20202E; color: #FFFFFF;
            border: none; border-radius: 6px; font-size: 13px;
        }
        #BtnAccept:hover { background-color: #35354A; }

        #BtnReject {
            background-color: #F0F0F0; color: #555555;
            border: none; border-radius: 6px; font-size: 13px;
        }
        #BtnReject:hover { background-color: #E4E4E4; }

        #BtnAccept:disabled, #BtnReject:disabled {
            background-color: #EDEDED; color: #AAAAAA;
        }
    )");

    // 按钮点了就把 username 带出去，上层不用再去问"是哪一行"
    connect(btnAccept, &QPushButton::clicked, this, [this]() {
        emit sendAccept(user);
        });
    connect(btnReject, &QPushButton::clicked, this, [this]() {
        emit sendReject(user);
        });
}

void RequestItem::setBusy(bool busy)
{
    btnAccept->setEnabled(!busy);
    btnReject->setEnabled(!busy);
}