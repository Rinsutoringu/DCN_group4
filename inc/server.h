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

// 客户端信息结构
struct ClientInfo
{
    SOCKET socket;
    std::string username;
    std::string group;
    bool muted = false;
    time_t last_activity;
};


extern std::map<std::string, std::set<std::string>> group_members;  // 群组成员
extern std::map<std::string, std::string> group_owners;  // 群组拥有者
extern std::map<std::string, ClientInfo> clients;  // 当前连接的用户列表
extern std::mutex client_mutex;  // 保护客户端列表的互斥锁
extern std::ofstream chatlog;  // 聊天记录文件

