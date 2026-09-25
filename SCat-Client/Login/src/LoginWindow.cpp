#include "../include/LoginWindow.h"

LoginWindow::LoginWindow(QWidget* parent):NoFrame(parent)
{
	NoFrame::Frameconfig config;
	config.background = QColor(255, 255, 255);
	config.titlebarcolor = QColor(255, 255, 255);
	config.borderRadius = 15;
	config.defaultsize = QSize(400,500);
	config.titlebarheight = 30;
	config.showlogo = false;
	config.showmax = false;
	config.showmin = false;
	config.showtext = false;
	config.showclose = true;
    config.btnsize = QSize(25, 25);
	this->setFrameconfig(config);
    this->setFixedSize(400, 500);
	initUI();
	initConnect();
}

LoginWindow::~LoginWindow()
{

}



void LoginWindow::initUI()
{
	QWidget* login = new QWidget(this);
	//控件
	lbLogo = new QLabel(login);
	lbLogo->setPixmap(QPixmap(":/Resource/icon/logo.png").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	lbLogo->setAlignment(Qt::AlignCenter);

	LogoText = new QLabel("SCat", login); 
	LogoText->setObjectName("LogoText"); 
	LogoText->setAlignment(Qt::AlignCenter);

	lbTexteg = new QLabel("Welcome come to SCat", login);
	lbTexteg->setObjectName("WelcomeEg");
	lbTexteg->setAlignment(Qt::AlignCenter);

	lbTextcn = new QLabel("欢迎来到SCat", login);
	lbTextcn->setObjectName("WelcomeCn");
	lbTextcn->setAlignment(Qt::AlignCenter);

	editaccount = new QLineEdit(login);
	editaccount->setPlaceholderText("Name");
	editaccount->setObjectName("InputEdit"); 
    editaccount->setFixedHeight(40);

	editpwd = new QLineEdit(login);
	editpwd->setPlaceholderText("Password");
	editpwd->setEchoMode(QLineEdit::Password); 
	editpwd->setObjectName("InputEdit");
    editpwd->setFixedHeight(40);

	checkrem = new QCheckBox("记住密码", login);

	btnLogin = new QPushButton("登录 login", login);
	btnLogin->setObjectName("BtnLogin"); 
	btnLogin->setCursor(Qt::PointingHandCursor); 
    btnLogin->setFixedHeight(44);

	btnRegister = new QPushButton("注册 register", login);
	btnRegister->setObjectName("BtnRegister"); 
	btnRegister->setCursor(Qt::PointingHandCursor);
    btnRegister->setFixedHeight(44);

	btnForgot = new QPushButton("忘记密码？点击这里", login);
	btnForgot->setObjectName("BtnForgot"); 
	btnForgot->setCursor(Qt::PointingHandCursor);
	//水平布局
    QHBoxLayout* logoLayout = new QHBoxLayout();
    logoLayout->addStretch(); 
    logoLayout->addWidget(lbLogo);
    logoLayout->addSpacing(0);
    logoLayout->addWidget(LogoText);
    logoLayout->addStretch(); 

    // 主垂直布局
    mainlayout = new QVBoxLayout(login);
    mainlayout->setContentsMargins(40, 20, 40, 20); 
    mainlayout->setSpacing(15); 

    // 依次添加
    mainlayout->addLayout(logoLayout);
    mainlayout->addSpacing(10);
    mainlayout->addWidget(lbTexteg);
    mainlayout->addWidget(lbTextcn);
    mainlayout->addSpacing(20); 
    mainlayout->addWidget(editaccount);
    mainlayout->addWidget(editpwd);
    mainlayout->addWidget(checkrem);
    //mainlayout->addSpacing(0);
    mainlayout->addWidget(btnLogin);
    mainlayout->addWidget(btnRegister);
    mainlayout->addStretch();
    mainlayout->addWidget(btnForgot);


    this->setMainWindow(login);

    QString qss = R"(
        #LogoText { font-size: 24px; font-weight: bold; color: #333; }
        #WelcomeEg { font-size: 14px; font-style: italic; color: #666; }
        #WelcomeCn { font-size: 16px; color: #333; margin-bottom: 10px; }

        #InputEdit {
            border: 1px solid #E0E0E0;
            border-radius: 8px; 
            padding-left: 15px;
            padding-right: 15px;
            font-size: 14px;
            background-color: #FAFAFA;
        }
        #InputEdit:focus {
            border: 1px solid #000000; 
            background-color: #FFFFFF;
        }
        #BtnLogin {
        background-color: #000000;
        color: #FFFFFF;
        border-radius: 8px;
        padding-left: 12px;
        padding-right: 12px;
        font-size: 14px;
        font-weight: bold;    
        }
        #BtnLogin:hover { background-color: #333333; }

        #BtnRegister {
            background-color: #EEEEEE;
            color: #333333;
            border: none;
            border-radius: 8px;
            padding-left: 12px;
            padding-right: 12px;
            font-size: 14px;
        }
        #BtnRegister:hover { background-color: #E0E0E0; }

        #BtnForgot {
            background: transparent;
            border: none;
            color: #666666;
            font-size: 12px;
        }
        #BtnForgot:hover { color: #000000; text-decoration: underline; }
    )";

    login->setStyleSheet(qss);
}

void LoginWindow::initConnect()
{
    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::sendRegisterClicked);
    connect(btnForgot, &QPushButton::clicked, this, &LoginWindow::sendForgotpwdClicked);
}

void LoginWindow::onLoginClicked()
{
    QString account = editaccount->text().trimmed();
    QString pwd = editpwd->text().trimmed();
    if (account.isEmpty() || pwd.isEmpty())
    {
        QMessageBox::warning(this, "提示", "账号或密码不能为空");
        return;
    }
    qDebug() << "尝试登录：" << account << pwd;
    emit sendLoginClicked(account, pwd);
}

void LoginWindow::onRegisterClicked()
{
    emit sendRegisterClicked();
}

void LoginWindow::onForgotpwdClicked()
{
    emit sendForgotpwdClicked();
}

QString LoginWindow::account() const
{
    return editaccount->text().trimmed();
}

QString LoginWindow::password() const
{
    return editpwd->text().trimmed();
}

bool LoginWindow::isRemember() const
{
    return checkrem->isChecked();
}

void LoginWindow::setSavedLogin(const QString& account, const QString& pwd)
{
    editaccount->setText(account);
    editpwd->setText(pwd);
    checkrem->setChecked(true);

    // 两个框都填好了，焦点直接给登录按钮，打开就能回车进去
    btnLogin->setFocus();
}
