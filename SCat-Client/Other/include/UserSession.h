#ifndef USERSESSION_H
#define USERSESSION_H

#include <QString>

class UserSession
{
private:
	QString uid;
	QString username;
	QString avatarPath;
	QString token;

	UserSession(QString a,QString b,QString c,QString d);

	~UserSession();

public:
	static UserSession& GetInstance();

};

#endif