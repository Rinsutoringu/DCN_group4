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
#define INACTIVITY_TIMEOUT 300 

// 使用宏处理用户指令，超级优雅！
#define HANDLE_COMMAND(cmd, func, ...) \
    if (msg.rfind(cmd, 0) == 0) { \
        func(__VA_ARGS__); \
        return; \
    }

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
    // 用户所在群名，用户不在群里就是空字符串
    std::string group = "";
    // 用户是否被塞口球
    bool muted = false;

    // 与服务器的最后连接时间
    time_t last_activity;
    // 是否是管理员
    bool is_admin = false;
    
    // 结构体构造函数
    ClientInfo(SOCKET sock, const std::string& usr, const std::string& grp_name, bool mute, time_t last_act)
        : socket(sock), username(usr), group(grp_name), muted(mute), last_activity(last_act) {};
    ClientInfo() {};
};

/*#####################声明全局变量#####################*/
// 群组拥有者
extern std::map<std::string, std::string> group_owners;
// 当前连接的用户列表
extern std::map<std::string, ClientInfo> clients;
// 保护客户端列表的互斥锁
extern std::mutex client_mutex;
// 聊天记录文件
extern std::ofstream chatlog;

/*#####################函数声明#####################*/

// TODO print到log文件的函数
void printToLogFile(const std::string& msg);


/**
 * 发送信息到指定客户端
 * @param sock 客户端socket
 * @param msg 发送的消息
 */
void sendToClient(SOCKET sock, const std::string& msg);

/**
 * 发送信息给有权限的客户端
 */
void sendToGroup(const std::string& msg);


/**
 * 发送消息给所有客户端
 * @param msg 发送的消息
 */
void broadcast(const std::string& msg);


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
 * @param usr 用户名
 * @return 如果是管理员返回true，否则返回false
 */
bool is_Admin(const std::string& usr);


/**
 * 判断用户是否是群组持有者
 * @param usr 用户名
 * @return 如果是群主返回true，否则返回false
 */
bool is_Owner(const std::string& usr);


/**
 * 处理用户输入
 * @param msg 用户输入的消息
 */
void handleUserInput(const std::string& usr, const std::string& grp, const std::string& msg);


/**
 * 处理建群请求
 * 现在只能建一个群，建群的人将成为Owner
 * 建群后获得权限
 * @param group 群名
 * @param usr 用户名
 */
void handleCreateGroup(const std::string& group, const std::string& usr);


/**
 * 处理踢人请求
 * 踢完后对应的人失去权限
 * @param usr 要踢谁？
 */
void handleKickGroup(const std::string& usr);


/**
 * 处理塞口球的请求
 * 被塞口球的人失去权限，等同未加群的人
 * @param usr 要禁言的人
 */
void handleMuteUser(const std::string& usr);


/**
 * 为特定用户解除禁言
 * 解除禁言后的人恢复权限
 * @param usr 被解除禁言的人
 */
void handleUnmuteUser(const std::string& usr);


/**
 * 解散群聊
 * 等同于将所有人都取消权限
 * 需要判断是否为Owner
 * @param usr 执行人名称
 */
void handleDismissGroup(const std::string& usr);

/**
 * 验证客户端报文合法性
 * @param usr 用户名
 * @param grp 群组名
 * @param client_sock 客户端socket
 */
void validateUserInput(const std::string& usr, const std::string& grp, SOCKET client_sock);


/**
 * 帮助函数
 */
void handleHelp();


/**
 * 加入群聊
 * 为当前用户启用权限
 */
void handleJoinGroup(const std::string& group, const std::string& usr);


/**
 * 离开群聊
 * 为当前用户禁用权限
 */
void handleLeaveGroup(const std::string& usr);


/**
 * 展示所有连接的客户端
 */
void handleShowAllClient();


/**
 * 展示当前用户状态
 * @param usr 用户名
 */
void handleShowUserStatus(const std::string& usr);


/**
 * 展示最近的20条聊天记录
 * 读取文件
 */
void handleShowHistory();


/**
 * 展示有权限的用户
 */
void handleShowGroupUser();

/**
 * 客户端与服务器断开连接指令
 * @param usr 要离线的用户名
 */
void handleQuit(const std::string& usr);

/**
 * 根据用户名获取特定的Client结构体
 * @param usr 需要获取结构体的用户名
 */
ClientInfo* getClient(const std::string& usr);