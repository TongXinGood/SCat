#include "../include/SettingsPage.h"
#include "../../Other/include/AppPath.h"
#include "../../Other/include/AvatarUtils.h"
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFontMetrics>
#include <QDebug>

SettingsPage::SettingsPage(QWidget* parent) : QWidget(parent)
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setObjectName("SettingsPage");

    initUI();
    initConnect();

    // 存储位置是本地的事，直接从 AppPath 读，不用上层喂
    setStoragePath(AppPath::dataRoot());
}

SettingsPage::~SettingsPage()
{
}

QWidget* SettingsPage::createCard()
{
    QWidget* card = new QWidget();
    card->setObjectName("Card");
    card->setAttribute(Qt::WA_StyledBackground, true);
    return card;
}

QWidget* SettingsPage::createRow(const QString& title, QWidget* right)
{
    QWidget* row = new QWidget();
    row->setObjectName("Row");
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setFixedHeight(60);

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(16);

    QLabel* lbTitle = new QLabel(title, row);
    lbTitle->setObjectName("RowTitle");
    lbTitle->setFixedWidth(52);

    layout->addWidget(lbTitle);
    layout->addWidget(right, 1);

    return row;
}

void SettingsPage::initUI()
{
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ============ 顶栏（跟聊天页的 header 同高同色） ============
    headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(70);
    headerWidget->setAttribute(Qt::WA_StyledBackground, true);
    headerWidget->setObjectName("SettingsHeader");

    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(24, 0, 24, 0);

    lbHeaderTitle = new QLabel("设置", headerWidget);
    lbHeaderTitle->setObjectName("HeaderTitle");

    headerLayout->addWidget(lbHeaderTitle);
    headerLayout->addStretch();

    QWidget* line = new QWidget(this);
    line->setObjectName("HeaderLine");
    line->setFixedHeight(1);
    line->setAttribute(Qt::WA_StyledBackground, true);

    // ============ 滚动区 ============
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setObjectName("SettingsScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 注意：这里绝对不能调 scroll->setStyleSheet("background: transparent;")。
    // 不带选择器的样式表会连同所有子控件一起刷，卡片和按钮的背景会全没。
    // 透明背景统一在下面的主样式表里用选择器处理
    scroll->viewport()->setAutoFillBackground(false);

    QWidget* content = new QWidget();
    content->setObjectName("SettingsContent");
    content->setAutoFillBackground(false);

    // 内容限宽居中，不让卡片横跨整个窗口
    QHBoxLayout* outer = new QHBoxLayout(content);
    outer->setContentsMargins(24, 24, 24, 32);
    outer->setSpacing(0);

    QWidget* column = new QWidget(content);
    column->setObjectName("SettingsColumn");
    column->setMaximumWidth(560);
    column->setMinimumWidth(340);

    QVBoxLayout* contentLayout = new QVBoxLayout(column);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);

    outer->addStretch(1);
    outer->addWidget(column, 4);
    outer->addStretch(1);

    // ---------- 卡片 1：头像 ----------
    QWidget* avatarCard = createCard();
    QVBoxLayout* avatarLayout = new QVBoxLayout(avatarCard);
    avatarLayout->setContentsMargins(20, 28, 20, 28);
    avatarLayout->setSpacing(16);

    lbAvatar = new QLabel(avatarCard);
    lbAvatar->setObjectName("AvatarLabel");
    lbAvatar->setFixedSize(96, 96);
    lbAvatar->setPixmap(AvatarUtils::load(QString(), 96));
    // 图由 AvatarUtils 裁好成 96x96 的圆形，所以不能开 setScaledContents，
    // 背景也必须透明，否则圆形四角会露出底色

    btnChangeAvatar = new QPushButton("更换头像", avatarCard);
    btnChangeAvatar->setObjectName("BtnGhost");
    btnChangeAvatar->setFixedSize(104, 34);
    btnChangeAvatar->setCursor(Qt::PointingHandCursor);

    avatarLayout->addWidget(lbAvatar, 0, Qt::AlignHCenter);
    avatarLayout->addWidget(btnChangeAvatar, 0, Qt::AlignHCenter);

    // ---------- 卡片 2：账号 / 昵称 ----------
    QWidget* infoCard = createCard();
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(0, 6, 0, 6);
    infoLayout->setSpacing(0);

    lbUsername = new QLabel("-", infoCard);
    lbUsername->setObjectName("RowValue");
    QWidget* rowUsername = createRow("账号", lbUsername);

    QWidget* rowLine = new QWidget(infoCard);
    rowLine->setObjectName("RowLine");
    rowLine->setFixedHeight(1);
    rowLine->setAttribute(Qt::WA_StyledBackground, true);

    QWidget* nickRight = new QWidget(infoCard);
    nickRight->setObjectName("Plain");
    QHBoxLayout* nickLayout = new QHBoxLayout(nickRight);
    nickLayout->setContentsMargins(0, 0, 0, 0);
    nickLayout->setSpacing(10);

    editNickname = new QLineEdit(nickRight);
    editNickname->setObjectName("InputEdit");
    editNickname->setFixedHeight(36);
    editNickname->setMaxLength(16);      // 跟数据库 nickname 字段对齐
    editNickname->setPlaceholderText("昵称");

    btnSaveNickname = new QPushButton("保存", nickRight);
    btnSaveNickname->setObjectName("BtnPrimary");
    btnSaveNickname->setFixedSize(68, 36);
    btnSaveNickname->setCursor(Qt::PointingHandCursor);

    nickLayout->addWidget(editNickname, 1);
    nickLayout->addWidget(btnSaveNickname);

    QWidget* rowNickname = createRow("昵称", nickRight);

    infoLayout->addWidget(rowUsername);
    infoLayout->addWidget(rowLine);
    infoLayout->addWidget(rowNickname);

    // ---------- 卡片 3：数据存储位置 ----------
    QWidget* storageCard = createCard();
    QVBoxLayout* storageLayout = new QVBoxLayout(storageCard);
    storageLayout->setContentsMargins(20, 20, 20, 20);
    storageLayout->setSpacing(10);

    QLabel* lbStorageTitle = new QLabel("数据存储位置", storageCard);
    lbStorageTitle->setObjectName("CardTitle");

    lbStoragePath = new QLabel(storageCard);
    lbStoragePath->setObjectName("PathValue");

    QLabel* lbStorageHint = new QLabel("聊天记录和头像缓存都存在这里，修改后需要重启程序生效",
        storageCard);
    lbStorageHint->setObjectName("Hint");
    lbStorageHint->setWordWrap(true);

    btnChangeStorage = new QPushButton("更改目录", storageCard);
    btnChangeStorage->setObjectName("BtnGhost");
    btnChangeStorage->setFixedSize(104, 34);
    btnChangeStorage->setCursor(Qt::PointingHandCursor);

    storageLayout->addWidget(lbStorageTitle);
    storageLayout->addWidget(lbStoragePath);
    storageLayout->addWidget(lbStorageHint);
    storageLayout->addSpacing(2);
    storageLayout->addWidget(btnChangeStorage, 0, Qt::AlignLeft);

    // ---------- 组装 ----------
    contentLayout->addWidget(avatarCard);
    contentLayout->addWidget(infoCard);
    contentLayout->addWidget(storageCard);
    contentLayout->addStretch();

    scroll->setWidget(content);

    rootLayout->addWidget(headerWidget);
    rootLayout->addWidget(line);
    rootLayout->addWidget(scroll, 1);

    // 全部样式集中在这里，每条都带类型 + ID 选择器。
    // 卡片设了 border，子控件会继承，所以每个子控件都要显式 border: none 盖掉
    this->setStyleSheet(R"(
        QWidget#SettingsPage    { background-color: #F2F0F5; }
        QWidget#SettingsHeader  { background-color: #FFFFFF; }
        QWidget#HeaderLine      { background-color: #E6E3EC; }
        QLabel#HeaderTitle      { font-size: 16px; font-weight: bold; color: #333333; }

        QScrollArea#SettingsScroll { background: transparent; border: none; }
        QWidget#SettingsContent    { background: transparent; }
        QWidget#SettingsColumn     { background: transparent; }

        QWidget#Card {
            background-color: #FFFFFF;
            border: 1px solid #ECE9F2;
            border-radius: 12px;
        }
        QWidget#Row     { background: transparent; border: none; }
        QWidget#Plain   { background: transparent; border: none; }
        QWidget#RowLine { background-color: #F2F2F2; border: none; }

        QLabel#AvatarLabel { background: transparent; border: none; }

        QLabel#CardTitle {
            font-size: 14px; font-weight: bold; color: #1A1A1A;
            background: transparent; border: none;
        }
        QLabel#RowTitle {
            font-size: 14px; color: #757575;
            background: transparent; border: none;
        }
        QLabel#RowValue {
            font-size: 14px; color: #1A1A1A;
            background: transparent; border: none;
        }
        QLabel#PathValue {
            font-size: 13px; color: #555555;
            background: transparent; border: none;
        }
        QLabel#Hint {
            font-size: 12px; color: #9E9E9E;
            background: transparent; border: none;
        }

        QLineEdit#InputEdit {
            border: 1px solid #E2E0E8;
            border-radius: 8px;
            padding-left: 12px;
            padding-right: 12px;
            font-size: 14px;
            color: #1A1A1A;
            background-color: #FAFAFC;
        }
        QLineEdit#InputEdit:focus {
            border: 1px solid #20202E;
            background-color: #FFFFFF;
        }

        QPushButton#BtnPrimary {
            background-color: #20202E;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton#BtnPrimary:hover    { background-color: #35354A; }
        QPushButton#BtnPrimary:pressed  { background-color: #16161F; }
        QPushButton#BtnPrimary:disabled { background-color: #E8E8ED; color: #AAAAAA; }

        QPushButton#BtnGhost {
            background-color: #F4F1FA;
            color: #3A3A4A;
            border: none;
            border-radius: 8px;
            font-size: 13px;
        }
        QPushButton#BtnGhost:hover    { background-color: #E7E1F5; }
        QPushButton#BtnGhost:pressed  { background-color: #DCD4F0; }
        QPushButton#BtnGhost:disabled { background-color: #F7F7F9; color: #BBBBBB; }
    )");
}

void SettingsPage::initConnect()
{
    connect(btnChangeAvatar, &QPushButton::clicked, this, &SettingsPage::onAvatarBtnClicked);
    connect(btnSaveNickname, &QPushButton::clicked, this, &SettingsPage::onSaveNicknameClicked);
    connect(btnChangeStorage, &QPushButton::clicked, this, &SettingsPage::onChangeStorageClicked);

    // 回车也能保存昵称
    connect(editNickname, &QLineEdit::returnPressed, this, &SettingsPage::onSaveNicknameClicked);
}

// ---------- 用户操作 ----------

void SettingsPage::onAvatarBtnClicked()
{
    // 打开 Windows 的文件选择对话框，默认定位到"图片"目录
    QString picDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString file = QFileDialog::getOpenFileName(this, "选择头像", picDir,
        "图片文件 (*.png *.jpg *.jpeg *.bmp)");

    if (file.isEmpty())
        return;      // 用户取消了

    btnChangeAvatar->setEnabled(false);
    btnChangeAvatar->setText("上传中");

    emit sendChangeAvatar(file);
}

void SettingsPage::onSaveNicknameClicked()
{
    QString nickname = editNickname->text().trimmed();
    if (nickname.isEmpty())
        return;

    btnSaveNickname->setEnabled(false);
    btnSaveNickname->setText("保存中");

    emit sendSaveNickname(nickname);
}

void SettingsPage::onChangeStorageClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择数据存储位置",
        fullStoragePath);

    if (dir.isEmpty() || dir == fullStoragePath)
        return;

    emit sendChangeStorage(dir);
}

// ---------- 给逻辑层调的接口 ----------

void SettingsPage::setUserInfo(const QString& username, const QString& nickname,
    const QString& avatar)
{
    lbUsername->setText(username);
    setNickname(nickname);
    setAvatar(avatar);
}

void SettingsPage::setAvatar(const QString& avatar)
{
    // 这里收到的是数据库 avatar 字段（文件名），不是路径。
    // 路径映射、缓存查找、圆形裁剪全在 AvatarUtils 里
    lbAvatar->setPixmap(AvatarUtils::load(avatar, 96));
}

void SettingsPage::setNickname(const QString& nickname)
{
    editNickname->setText(nickname);
}

void SettingsPage::setStoragePath(const QString& path)
{
    fullStoragePath = path;

    // 路径经常很长，中间打省略号，完整的放 tooltip 里
    QFontMetrics fm(lbStoragePath->font());
    lbStoragePath->setText(fm.elidedText(path, Qt::ElideMiddle, 480));
    lbStoragePath->setToolTip(path);
}

void SettingsPage::onNicknameSaved(bool ok, const QString& reason)
{
    btnSaveNickname->setEnabled(true);
    btnSaveNickname->setText("保存");

    if (!ok)
        qDebug() << "save nickname failed:" << reason;
}

void SettingsPage::onAvatarUploaded(bool ok, const QString& reason)
{
    btnChangeAvatar->setEnabled(true);
    btnChangeAvatar->setText("更换头像");

    if (!ok)
        qDebug() << "upload avatar failed:" << reason;
}