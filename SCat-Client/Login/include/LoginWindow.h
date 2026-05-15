#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include "../../NoFrame.h" 
#include <QLineEdit> 
#include <QPushButton>
#include <QWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class LoginWindow :public NoFrame 
{
	Q_OBJECT
public:
	explicit LoginWindow(QWidget* parent = nullptr);
	~LoginWindow();
signals:
	void sendLoginClicked(const QString& account , const QString& pwd);
	void sendRegisterClicked();
	void sendForgotpwdClicked();
private slots:
	void onLoginClicked();
	void onRegisterClicked();
	void onForgotpwdClicked();
private:
	void initUI();
	void initConnect();
private:
	QLabel* lbLogo;
	QLabel* LogoText;

	QLabel* lbTextcn;
	QLabel* lbTexteg;

	QLineEdit* editaccount;
	QLineEdit* editpwd;

	QCheckBox* checkrem;

	QPushButton* btnLogin;
	QPushButton* btnRegister;
	QPushButton* btnForgot;

	QVBoxLayout* mainlayout;
};

#endif // LOGINWINDOW_H