#include "../include/RegisterWindow.h"
#include <QMessageBox>

RegisterWindow::RegisterWindow(QWidget* parent):NoFrame(parent)
{
	NoFrame::Frameconfig config;
	config.background = QColor(255, 255, 255);
	config.titlebarcolor = QColor(255, 255, 255);
	config.borderRadius = 15;
	config.defaultsize = QSize(400, 500);
	config.titlebarheight = 30;
	config.showlogo = false;
	config.showmax = false;
	config.showmin = false;
	config.showtext = false;
	config.showclose = true;
    config.btnsize = QSize(25, 25);
	this->setFrameconfig(config);
    this->setFixedSize(400, 500);
    initUi();
	initconnect();

}

RegisterWindow::~RegisterWindow()
{

}


void RegisterWindow::initUi()
{
    QWidget* reg = new QWidget(this);
    reg->setObjectName("reg");

    // Logo 
    Logo = new QLabel(reg);
    Logo->setPixmap(QPixmap(":/Resource/icon/logo.png").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    Logo->setAlignment(Qt::AlignCenter);
    //软件名称
    LogoText = new QLabel("SCat", reg);
    LogoText->setObjectName("LogoText");
    LogoText->setAlignment(Qt::AlignCenter);

    //欢迎语句
    texteg1 = new QLabel("Join Scat !", reg);
    texteg1->setObjectName("JoinText"); 
    texteg1->setAlignment(Qt::AlignCenter);
    texteg2 = new QLabel("Create your Account", reg);
    texteg2->setObjectName("CreateText"); 
    texteg2->setAlignment(Qt::AlignCenter);

    // 4. 输入框
    account = new QLineEdit(reg);
    account->setPlaceholderText("Name");
    account->setObjectName("InputEdit");
    pwd = new QLineEdit(reg);
    pwd->setPlaceholderText("Password");
    pwd->setEchoMode(QLineEdit::Password);
    pwd->setObjectName("InputEdit");

    //按钮
    btnregister = new QPushButton("注册 Let's Chat", reg); 
    btnregister->setObjectName("BtnRegister"); 
    btnregister->setCursor(Qt::PointingHandCursor);
    btnreturn = new QPushButton("返回", reg);
    btnreturn->setObjectName("BtnReturn"); 
    btnreturn->setCursor(Qt::PointingHandCursor);

    //布局
    QHBoxLayout* logoLayout = new QHBoxLayout();
    logoLayout->addStretch();
    logoLayout->addWidget(Logo);
    logoLayout->addSpacing(10);
    logoLayout->addWidget(LogoText);
    logoLayout->addStretch();

    mainLayout = new QVBoxLayout(reg);
    mainLayout->setContentsMargins(40, 30, 40, 20);
    mainLayout->setSpacing(15);

    mainLayout->addLayout(logoLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(texteg1);
    mainLayout->addWidget(texteg2);
    mainLayout->addSpacing(25);
    mainLayout->addWidget(account);
    mainLayout->addWidget(pwd);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(btnregister);
    mainLayout->addStretch();
    mainLayout->addWidget(btnreturn);

    this->setMainWindow(reg);

    QString qss = R"(
        /* 全局背景设为白色 */
        QWidget#reg {
            background-color: #FFFFFF;
        }

        /* 1. SCat 标题 */
        #LogoText { 
            font-size: 28px; 
            font-weight: bold; 
            color: #000000; 
            margin-left: 5px; /* 微调跟logo的间距 */
        }

        /* 2. Join Scat ! - 图片中是斜体、稍大 */
        #JoinText { 
            font-size: 16px; 
            color: #333; 
            margin-bottom: 2px;
        }

        /* 3. Create your Account - 图片中是斜体、稍小 */
        #CreateText { 
            font-size: 16px; 
            font-style: italic; /* 关键：斜体 */
            color: #333; 
            margin-bottom: 15px; 
        }

        /* 4. 输入框 - 胶囊形状，极淡的边框 */
        #InputEdit {
            border: 1px solid #E0E0E0; /* 很浅的灰边框 */
            border-radius: 1px;       /* 大圆角 */
            padding: 10px 15px;         /* 左右留白 */
            background-color: #FAFAFA; /* 纯白背景 */
            font-size: 14px;
            color: #333333;
        }
        #InputEdit:focus {
            border: 1px solid #000000; /* 聚焦时边框稍深 */
            background-color:#FFFFFF;
        }

      
        #BtnRegister {
            background-color: #F2F2F2; 
            color: #333333;            
            border: none;
            border-radius: 1px;       
            font-size: 16px;
            font-weight: bold;
            min-height: 40px;   
        }
        #BtnRegister:hover {
            background-color: #E0E0E0; /* 悬停变色 */
        }
        #BtnRegister:pressed {
            background-color: #D6D6D6; /* 按下变色 */
        }

        /* 6. 底部返回按钮 - 小灰字 */
        #BtnReturn {
            background: transparent;
            border: none;
            color: #666666;
            font-size: 13px;
        }
        #BtnReturn:hover {
            color: #000000;
        }
    )";
    reg->setStyleSheet(qss);
}

void RegisterWindow::initconnect()
{
	connect(btnregister, &QPushButton::clicked, this, &RegisterWindow::onRegisterClicked);
	connect(btnreturn, &QPushButton::clicked, this, &RegisterWindow::onReturnClicked);
}

void RegisterWindow::onRegisterClicked()
{
	QString account1 = account->text().trimmed();
	QString pwd1=pwd->text().trimmed();
	if (account1.isEmpty() || pwd1.isEmpty())
	{
        QMessageBox::warning(this, "提示", "账号或密码不能为空");
		return;
	}
	emit sendRegisterClicked(account1, pwd1);
}

void RegisterWindow::onReturnClicked()
{
	emit sendReturnClicked();
}
