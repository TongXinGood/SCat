#include "NoFrame.h"
#include "Other/include/Theme.h"
#include <QImage>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

// 老一点的 Windows SDK 里没有这两个常量，自己补上
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#endif

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

// 把一张深色图标染成指定颜色。按深浅换透明度：原来越黑的地方越不透明，
// 白底的图标也能用，白底会变透明
static QIcon tintIcon(const QString& path, const QColor& color)
{
	QImage img = QImage(path).convertToFormat(QImage::Format_ARGB32);

	for (int y = 0; y < img.height(); ++y) {
		QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
		for (int x = 0; x < img.width(); ++x) {
			int alpha = qAlpha(line[x]) * (255 - qGray(line[x])) / 255;
			line[x] = qRgba(color.red(), color.green(), color.blue(), alpha);
		}
	}

	return QIcon(QPixmap::fromImage(img));
}

// 标题栏按钮平时的图标。图标本身是黑色的，深色模式下黑底上看不见，染成浅灰
static QIcon titleIcon(const QString& path)
{
	if (!Theme::isDark())
		return QIcon(path);

	return tintIcon(path, QColor("#B8B5C4"));
}
NoFrame::Frameconfig::Frameconfig()
	:background(255, 255, 255)
	, borderRadius(10)
	, defaultsize(800, 600)
	, titlebarheight(30)
	, titlebarcolor(255, 255, 255)
	, showlogo(false)
	, showclose(true)
	, showmin(true)
	, showmax(false)
	, showtext(false)
	, logoimage(":/Resource/icon/logo.png")
	, minicon(":/Resource/icon/min.png")
	, maxicon(":/Resource/icon/max.png")
	, closeicon(":/Resource/icon/close.png")
	, logosize(20, 20)
	, btnsize(16, 16)
{
}

NoFrame::NoFrame(QWidget* parent) :FramelessWindow(parent), ismax(false) {
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
	applyDarkFrame();
}

NoFrame::~NoFrame() {
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

	// logo 和标题文字不接收鼠标，点在它们上面也能拖动窗口。
	// 不加的话，拖动时 Windows 问"这里是不是标题栏"，会因为点到了子控件而说"不是"
	lbLogo->setAttribute(Qt::WA_TransparentForMouseEvents);
	lbTitle->setAttribute(Qt::WA_TransparentForMouseEvents);

	btnMin = new TitleButton(titleBar);
	btnMin->setObjectName("btnMin");
	btnMax = new TitleButton(titleBar);
	btnMax->setObjectName("btnMax");
	btnClose = new TitleButton(titleBar);
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

void NoFrame::applyDarkFrame()
{
#ifdef Q_OS_WIN
	// Win11 会在无边框窗口外面画一圈 1 像素的细边框，默认是浅灰色，
	// 深色模式下贴着深色窗口很扎眼，换成跟分割线一样的深灰。
	// Win10 没有这圈边框，这两个设置会被忽略，不影响
	if (!Theme::isDark())
		return;

	HWND hwnd = HWND(winId());
	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

	QColor border = Theme::bg(QColor("#E4E4E4"));
	COLORREF color = RGB(border.red(), border.green(), border.blue());
	DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
#endif
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
	// 各个窗口传进来的还是浅色，这里按当前主题换一下
	QColor titleColor = Theme::bg(config.titlebarcolor);
	QColor bgColor = Theme::bg(config.background);

	QString titleColorStr = QString("rgb(%1, %2, %3)")
		.arg(titleColor.red())
		.arg(titleColor.green())
		.arg(titleColor.blue());
	QString bgColorStr = QString("rgb(%1, %2, %3)")
		.arg(bgColor.red())
		.arg(bgColor.green())
		.arg(bgColor.blue());


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

	// 悬停压的那层半透明黑，深色模式下会换成半透明白
	this->setStyleSheet(Theme::css(qss));
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

	static_cast<TitleButton*>(btnMin)->setIcons(titleIcon(config.minicon), QIcon());
	static_cast<TitleButton*>(btnMax)->setIcons(titleIcon(config.maxicon), QIcon());
	static_cast<TitleButton*>(btnClose)->setIcons(titleIcon(config.closeicon), tintIcon(config.closeicon, Qt::white));

	lbLogo->setFixedSize(config.logosize);
	QPixmap logoMap(config.logoimage);
	lbLogo->setPixmap(logoMap.scaled(config.logosize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

}

void NoFrame::setMainWindow(QWidget* widget)
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

void NoFrame::setTitleStyle(const QString& qss)
{
	lbTitle->setStyleSheet(Theme::css(qss));
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
		btnMax->setIcon(titleIcon(config.maxicon));

	}
	else {
		this->showMaximized();
	}
}