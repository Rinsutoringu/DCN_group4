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

// 使用std作用域
using namespace std;

// 客户端信息结构
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

// 全局变量声明
extern std::map<std::string, std::set<std::string>> group_members;  // 群组成员
extern std::map<std::string, std::string> group_owners;  // 群组拥有者
extern std::map<std::string, ClientInfo> clients;  // 当前连接的用户列表
extern std::mutex client_mutex;  // 保护客户端列表的互斥锁
extern std::ofstream chatlog;  // 聊天记录文件

// 函数声明
std::string xorCipher(const std::string& data);
std::string getTimestamp();

// 工具函数
void sendToClient(SOCKET sock, const std::string& msg);
void broadcast(const std::string& group, const std::string& msg, const std::string& except = "");
void checkInactiveUsers();
void handleClient(SOCKET client_sock);
bool is_Admin(const std::string& username);
bool is_Owner(const std::string& username);

// 处理指令
void handleLeaveGroup(const std::string& group, const std::string& username);
void handleMuteUser(const std::string& group, const std::string& username);
