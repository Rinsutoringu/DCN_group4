#pragma once

#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <exception>

#define XOR_KEY 0xAB
#define MAX_CLIENTS 100
#define INACTIVITY_TIMEOUT 300 // 5 minutes in seconds


/*#####################客户端信息结构体#####################*/

/**
 * 客户端信息结构体
 */
struct ClientInfo
{
    // 客户端socket
    SOCKET socket;
    // 用户名
    std::string username;
    // 用户所在群名
    std::string group;
    // 用户是否被塞口球
    bool muted = false;
    // 与服务器的最后连接时间
    time_t last_activity;
    // 是否是管理员
    bool is_admin = false;
};

/*#####################声明全局变量#####################*/
// 群组成员
extern std::map<std::string, std::set<std::string>> group_members;
// 群组拥有者
extern std::map<std::string, std::string> group_owners;
// 当前连接的用户列表
extern std::map<std::string, ClientInfo> clients;
// 保护客户端列表的互斥锁
extern std::mutex client_mutex;
// 聊天记录文件
extern std::ofstream chatlog;

/*#####################函数声明#####################*/

/**
 * 发送信息到客户端
 * @param sock 客户端socket
 * @param msg 发送的消息
 */
void sendToClient(SOCKET sock, const std::string& msg);


/**
 * 指定群组，广播消息到所有在该群组中的客户端
 * @param group 群组名称
 * @param msg 发送的消息
 * @param except 排除的用户
 */
void broadcast(const std::string& group, const std::string& msg, const std::string& except = "");


/**
 * 检查不活跃的用户
 */
void checkInactiveUsers();


/**
 * 处理客户端连接
 * @param client_sock 客户端socket
 */
void handleClient(SOCKET client_sock);


/**
 * 获取当前时间戳
 * @return 当前时间戳字符串
 */
std::string getTimestamp();


/**
 * XOR加密函数
 * @param data 要加密的数据
 * @return 加密后的数据
 */
std::string xorCipher(const std::string& data);


/**
 * 判断用户是否是管理员
 * 暂时不用
 * @param username 用户名
 * @return 如果是管理员返回true，否则返回false
 */
bool is_Admin(const std::string& username);


/**
 * 判断用户是否是群组持有者
 * @param username 用户名
 * @return 如果是群主返回true，否则返回false
 */
bool is_Owner(const std::string& username);


/**
 * 处理踢人请求
 * @param group 目标群组
 * @param username 要踢谁？
 */
void handleLeaveGroup(const std::string& group, const std::string& username);


/**
 * 处理塞口球的请求
 * @param username 给谁塞口球？
 */
void handleMuteUser(const std::string& username);


/**
 * 处理建群请求
 * @param group 群名
 * @param username 用户名
 */
void handleCreateGroup(const std::string& group, const std::string& username);

/**
 * 解除用户静音
 * @param username 用户名
 */
void handleUnmuteUser(const std::string& username);

