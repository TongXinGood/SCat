#pragma once
#include <QObject>

//定义包头的格式
//4字节Body 2字节类型 
constexpr int HEADER_SIZE = 6;
/*
	1-9为心跳包
	10-20
*/
enum MsgType :quint16 {
	//心跳包
	MSG_HEARTBEAT = 1,
	MSG_HEARTBEAT_RESP = 2,

	//登陆、注册等服务请求
	MSG_LOGIN_REQ = 10,
	MSG_LOGIN_RESP = 11,
	MSG_REG_REQ = 12,
	MSG_REG_RESP = 13,

	//消息类型包
	MSG_CHAT_REQ = 20,
	MSG_CHAT_RESP = 21,
	MSG_CHAT_PUSH = 22,

	//好友相关
	MSG_FRIEND_LIST_REQ = 30,
	MSG_FRIEND_LIST_RESP = 31,

	MSG_FRIEND_STATUS_PUSH = 32,
};

constexpr quint32 MAX_BODY_SIZE = 1024 * 1024;

enum ErrCode : int {
	ERR_OK = 0,
	ERR_USER_EXIST = 1,   // 注册：用户名已存在
	ERR_USER_NOT_FOUND = 2,   // 登录：用户不存在
	ERR_WRONG_PASSWORD = 3,   // 登录：密码错误
	ERR_ALREADY_ONLINE = 4,   // 登录：该账号已在别处登录
	ERR_INVALID_PARAM = 5,    // 参数格式不合法
	ERR_NOT_LOGIN = 6,		  // 没登录就请求需要身份的接口

	ERR_DB_ERROR = 99,   // 数据库操作失败
};
