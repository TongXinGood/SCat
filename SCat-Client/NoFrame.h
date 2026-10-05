#ifndef NOFRAME_H
#define NOFRAME_H

#include "framelesswindow.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>

class NoFrame : public FramelessWindow
{
	Q_OBJECT
public:
	//边框设置
	struct  Frameconfig
	{
		//标题栏UI部分
		QColor background;  //背景颜色
		int borderRadius;	  //圆角大小
		QSize defaultsize; //默认大小
		int titlebarheight; //标题栏高度
		QColor titlebarcolor; // 标题栏颜色
		//标题栏显示部分
		bool showlogo; //是否显示Logo
		bool showmin; //是否显示最小化
		bool showmax; //是否显示最大化
		bool showclose; //是否显示关闭按钮
		bool showtext; //是否显示文本
		//图片路径
		QString logoimage; //logo图片
		QString minicon; //最小化图片
		QString maxicon; //最大化图片
		QString closeicon; //关闭图片
		//图片大小
		QSize logosize; //logo大小
		QSize btnsize; //按钮大小
		Frameconfig();
	};
public:
	//构造函数，析构函数
	explicit NoFrame(QWidget* parent = nullptr);
	~NoFrame();
	//设置窗口数值
	void setFrameconfig(const Frameconfig &config);
	void setMainWindow(QWidget* widget);
	void setWindowTitle(const QString& title);
	//标题文字的样式。默认是小字（主窗口左上角的 "SCat"）；
	// 弹窗想把大标题放进标题栏、跟右边的关闭按钮对齐，就用它把字调大
	void setTitleStyle(const QString & qss);
private slots:
	void onbtncloseClick();
	void onbtnminClick();
	void onbtnmaxClick();

private:
	void initUi();
	void updateStyle();
	void changeIconsize();

private:
	QWidget* centralContainer;//包裹整个窗口内容的容器 (用于做圆角背景)
	QVBoxLayout* mainlayout;//布局
	QWidget* titleBar;         // 标题栏容器
	QHBoxLayout* titleLayout;  // 标题栏水平布局

	QLabel* lbLogo;           // Logo
	QLabel* lbTitle;          // 标题文字

	QPushButton* btnMin;       // 最小化
	QPushButton* btnMax;       // 最大化
	QPushButton* btnClose;     // 关闭

	QWidget* contentArea;      // 专门用来放 setMainWidget 进来的内容的占位符

	Frameconfig config;        // 保存当前的配置
	bool ismax;     // 记录当前是否最大化，用于切换图标
};

#endif // NOFRAME_H