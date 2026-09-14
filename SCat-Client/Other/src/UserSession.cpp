#include "../include/UserSession.h"

UserSession& UserSession::GetInstance()
{
	// C++11 起，函数内 static 的初始化是线程安全的
	static UserSession instance;
	return instance;
}

void UserSession::setUser(const QString& name, const QString& nick, const QString& avatar)
{
	userName = name;
	nickName = nick;
	avatarPath = avatar;
}

void UserSession::clear()
{
	userName.clear();
	nickName.clear();
	avatarPath.clear();
}