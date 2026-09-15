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

	MSG_OFFLINE_PUSH = 23,   // 上线后服务端补发的离线消息（一次一批）
	MSG_OFFLINE_ACK = 24,    // 客户端确认已存好，服务端据此删表

	//好友相关
	MSG_FRIEND_LIST_REQ = 30,
	MSG_FRIEND_LIST_RESP = 31,

	MSG_FRIEND_STATUS_PUSH = 32,

	//加好友
	MSG_SEARCH_USER_REQ = 33,
	MSG_SEARCH_USER_RESP = 34,
	MSG_FRIEND_ADD_REQ = 35,
	MSG_FRIEND_ADD_RESP = 36,

	MSG_FRIEND_REQ_PUSH = 37,       // 服务端推：有人申请加你
	MSG_FRIEND_REQ_LIST_REQ = 38,   // 拉待处理申请列表
	MSG_FRIEND_REQ_LIST_RESP = 39,
	MSG_FRIEND_HANDLE_REQ = 40,     // 同意 / 拒绝
	MSG_FRIEND_HANDLE_RESP = 41,
	MSG_FRIEND_LIST_CHANGED = 42,   // 好友列表变了，客户端收到就重新拉一次

	//设置
	MSG_SET_NICKNAME_REQ = 50,
	MSG_SET_NICKNAME_RESP = 51,

	MSG_SET_AVATAR_REQ = 52,
	MSG_SET_AVATAR_RESP = 53,
	MSG_GET_AVATAR_REQ = 54,
	MSG_GET_AVATAR_RESP = 55,
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
	ERR_ALREADY_FRIEND = 7,   // 已经是好友了
	ERR_CANNOT_ADD_SELF = 8,  // 不能加自己
	ERR_REQUEST_NOT_FOUND = 9,   // 这条申请不存在，或已经被处理过了
	ERR_DB_ERROR = 99,   // 数据库操作失败
};

// 搜索结果里，这个人跟我是什么关系。
// 没有"已申请"这一档 —— 只要对方还没同意就允许重新申请
enum RelationType : int {
	RELATION_STRANGER = 0,   // 陌生人，可以申请
	RELATION_FRIEND = 1,     // 已经是好友
	RELATION_SELF = 2,       // 是你自己
};

enum HandleAction : int {
	HANDLE_ACCEPT = 1,
	HANDLE_REJECT = 2,
};