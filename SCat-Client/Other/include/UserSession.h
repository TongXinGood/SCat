#ifndef USERSESSION_H
#define USERSESSION_H

#include <QString>

// 全局单例：保存"我是谁"。
// 登录成功后由 AppController 写入，之后聊天、加好友等模块都从这里取自己的 username。
class UserSession
{
public:
	static UserSession& GetInstance();

	// 登录成功后调用
	void setUser(const QString& name, const QString& nick, const QString& avatar);

	// 退出登录 / 掉线时调用，清空身份
	void clear();

	bool isLogined() const { return !userName.isEmpty(); }

	QString username() const { return userName; }     // 唯一标识，登录用，永不改变
	QString nickname() const { return nickName; }     // 显示名，用户可以随便改
	QString avatar() const { return avatarPath; }

private:
	UserSession() = default;
	~UserSession() = default;

	// 单例，禁止拷贝
	UserSession(const UserSession&) = delete;
	UserSession& operator=(const UserSession&) = delete;

private:
	QString userName;       // 空 = 未登录
	QString nickName;
	QString avatarPath;
};

#endif // USERSESSION_H