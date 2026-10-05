#include "NoFrame.h"
#include <QImage>

static const int kTitleBtnWidth = 46;     // 标题栏按钮的宽度，跟 Windows 自带的一样

// 标题栏上的按钮。可以给鼠标悬停时单独配一个图标：
// 关闭按钮悬停是红底，上面的 × 得变成白色才看得清，跟 Windows 自带的一样
class TitleButton : public QPushButton
{
public:
	explicit TitleButton(QWidget* parent = nullptr) : QPushButton(parent) {}

	void setIcons(const QIcon& normal, const QIcon& hover)
	{
		normalIcon = normal;
		hoverIcon = hover;
		setIcon(underMouse() && !hover.isNull() ? hover : normal);
	}

protected:
	void enterEvent(QEnterEvent* event) override
	{
		if (!hoverIcon.isNull())
			setIcon(hoverIcon);
		QPushButton::enterEvent(event);
	}

	void leaveEvent(QEvent* event) override
	{
		setIcon(normalIcon);
		QPushButton::leaveEvent(event);
	}

private:
	QIcon normalIcon;
	QIcon hoverIcon;
};

// 把一张深色图标染成白色。按深浅换透明度：原来越黑的地方越不透明，
// 白底的图标也能用，白底会变透明
static QIcon whiteIcon(const QString& path)
{
	QImage img = QImage(path).convertToFormat(QImage::Format_ARGB32);

	for (int y = 0; y < img.height(); ++y) {
		QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
		for (int x = 0; x < img.width(); ++x) {
			int alpha = qAlpha(line[x]) * (255 - qGray(line[x])) / 255;
			line[x] = qRgba(255, 255, 255, alpha);
		}
	}

	return QIcon(QPixmap::fromImage(img));
}
NoFrame::Frameconfig::Frameconfig()
	:background(255,255,255)
	,borderRadius(10)
	,defaultsize(800,600)
	,titlebarheight(30)
	,titlebarcolor(255,255,255)
	,showlogo(false)
	,showclose(true)
	,showmin(true)
	,showmax(false)
	,showtext(false)
	,logoimage(":/Resource/icon/logo.png")
	,minicon(":/Resource/icon/min.png")
	,maxicon(":/Resource/icon/max.png")
	,closeicon(":/Resource/icon/close.png")
	,logosize(20,20)
	,btnsize(16,16)
{}

NoFrame::NoFrame(QWidget* parent) :FramelessWindow(parent) ,ismax(false){
	//防止野指针
	centralContainer = nullptr;
	mainlayout = nullptr;
	titleBar = nullptr;
	titleLayout = nullptr;
	lbLogo = nullptr;
	lbTitle = nullptr;
	btnMin = nullptr;
	btnMax = nullptr;
	btnClose = nullptr;
	contentArea = nullptr;
	//初始化`
	initUi();
	setFrameconfig(config);
}

NoFrame::~NoFrame(){
}

void NoFrame::initUi() {
	centralContainer = new QWidget(this);
	centralContainer->setObjectName("centralContainer");
	this->setCentralWidget(centralContainer);
	mainlayout = new QVBoxLayout(centralContainer);
	mainlayout->setContentsMargins(0, 0, 0, 0);
	mainlayout->setSpacing(0);
	titleBar = new QWidget(this);
	titleBar->setObjectName("titleBar");
	titleLayout = new QHBoxLayout(titleBar);
	titleLayout->setContentsMargins(10, 0, 0, 0);
	titleLayout->setSpacing(10);
	lbLogo = new QLabel(titleBar);
	lbLogo->setObjectName("lbLogo");
	lbTitle = new QLabel(titleBar);
	lbTitle->setObjectName("lbTitle");
	btnMin = new QPushButton(titleBar);
	btnMin->setObjectName("btnMin");
	btnMax = new QPushButton(titleBar);
	btnMax->setObjectName("btnMax");
	btnClose = new QPushButton(titleBar);
	btnClose->setObjectName("btnClose");

	titleLayout->addWidget(lbLogo);
	titleLayout->addWidget(lbTitle);
	titleLayout->addStretch(1);
	// 三个按钮单独一组、彼此之间不留空隙，紧贴窗口右边，
	// 悬停的底色才能连成一片、顶到窗口的上边和右边
	QHBoxLayout* btnLayout = new QHBoxLayout();
	btnLayout->setContentsMargins(0, 0, 0, 0);
	btnLayout->setSpacing(0);
	btnLayout->addWidget(btnMin);
	btnLayout->addWidget(btnMax);
	btnLayout->addWidget(btnClose);
	titleLayout->addLayout(btnLayout);

	contentArea = new QWidget(this);
	contentArea->setObjectName("contentArea");

	mainlayout->addWidget(titleBar);
	mainlayout->addWidget(contentArea, 1);

	this->setTitleBar(titleBar);

	connect(btnMin, &QPushButton::clicked, this, &NoFrame::onbtnminClick);
	connect(btnMax, &QPushButton::clicked, this, &NoFrame::onbtnmaxClick);
	connect(btnClose, &QPushButton::clicked, this, &NoFrame::onbtncloseClick);
}

void NoFrame::setFrameconfig(const NoFrame::Frameconfig& config)
{
	this->config = config;
	this->resize(config.defaultsize);
	titleBar->setFixedHeight(config.titlebarheight);
	lbLogo->setVisible(config.showlogo);
	lbTitle->setVisible(config.showtext);
	btnMin->setVisible(config.showmin);    
	btnMax->setVisible(config.showmax);     
	btnClose->setVisible(config.showclose);
	updateStyle();    
	changeIconsize();
}

void NoFrame::updateStyle()
{
	QString titleColorStr = QString("rgb(%1, %2, %3)")
		.arg(config.titlebarcolor.red())
		.arg(config.titlebarcolor.green())
		.arg(config.titlebarcolor.blue());
	QString bgColorStr = QString("rgb(%1, %2, %3)")
		.arg(config.background.red())
		.arg(config.background.green())  
		.arg(config.background.blue());


	QString qss = QString(
		"#titleBar { "
		"   background-color: %1; "
		"   border-top-left-radius: %3px; "   
		"   border-top-right-radius: %3px; " 
		"} "

		// 窗口中心背景样式 
		"#centralContainer { "
		"   background-color: %2; "
		"   border-radius: %3px; "            // 四周圆角
		"} "

		//按钮基础样式
		"QPushButton { "
		"   border: none; "
		"   background: transparent; "
		"} "


		"#btnMin:hover, #btnMax:hover { "
		"   background-color: rgba(0, 0, 0, 30); "
		"} "

		"#btnMin:pressed, #btnMax:pressed { "
		"   background-color: rgba(0, 0, 0, 50); "
		"} "

		// 红底不用自己画圆角：窗口的圆角是 Win11 系统切的，
		// 整个窗口连同这块红底会被一起切圆，自己再画一个反而对不齐
		"#btnClose:hover { "
		"   background-color: rgb(232, 17, 35); "
		"} "

		"#btnClose:pressed { "
		"   background-color: rgb(241, 112, 122); "
		"} "
	)
		.arg(titleColorStr)  
		.arg(bgColorStr)
		.arg(config.borderRadius); 

	this->setStyleSheet(qss);
}

void NoFrame::changeIconsize()
{
	// 按钮跟标题栏一样高、宽度固定，悬停的底色才能把整块填满。
	// btnsize 只管里面图标画多大
	QSize btnSize(kTitleBtnWidth, config.titlebarheight);
	btnMin->setFixedSize(btnSize);
	btnMax->setFixedSize(btnSize);
	btnClose->setFixedSize(btnSize);

	btnMin->setIconSize(config.btnsize);       // 设置显示大小
	btnMax->setIconSize(config.btnsize);
	btnClose->setIconSize(config.btnsize);

	static_cast<TitleButton*>(btnMin)->setIcons(QIcon(config.minicon), QIcon());
	static_cast<TitleButton*>(btnMax)->setIcons(QIcon(config.maxicon), QIcon());
	static_cast<TitleButton*>(btnClose)->setIcons(QIcon(config.closeicon), whiteIcon(config.closeicon));

	lbLogo->setFixedSize(config.logosize);
	QPixmap logoMap(config.logoimage);
	lbLogo->setPixmap(logoMap.scaled(config.logosize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

}

void NoFrame::setMainWindow(QWidget *widget)
{
	QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(contentArea->layout());
	if (layout == nullptr) {
		layout = new QVBoxLayout(contentArea);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
	}
	layout->addWidget(widget);
}

void NoFrame::setWindowTitle(const QString& title)
{
	lbTitle->setText(title);
	FramelessWindow::setWindowTitle(title);
}

void NoFrame::onbtncloseClick()
{
	this->close();
}

void NoFrame::onbtnminClick() 
{
	this->showMinimized();
}

void NoFrame::onbtnmaxClick()
{
	if (this->isMaximized()) {
		this->showNormal();
		btnMax->setIcon(QIcon(config.maxicon));

	}
	else {
		this->showMaximized();
	}
}