#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "server.h"
#pragma comment(lib, "ws2_32.lib")

// 定义全局变量
std::map<std::string, std::string> group_owners;
std::map<std::string, ClientInfo> clients;
std::mutex client_mutex;
std::ofstream chatlog;

// 使用std作用域
using namespace std;

string xorCipher(const string& data) {
    string res = data;
    for (char& c : res) c ^= XOR_KEY;
    return res;
}


string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm tm; localtime_s(&tm, &t);
    ostringstream oss;
    oss << put_time(&tm, "[%H:%M]");
    return oss.str();
}



void handleClient(SOCKET client_sock) {
    string usr, grp, msg;
    char buffer[1024];

    // 获取客户端报文并进行处理
    {
        lock_guard<mutex> lock(client_mutex);
        
        int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) return;
        // 解密
        istringstream iss(xorCipher(std::string(buffer, len)));
        // 流式切割报文
        
        iss >> usr >> grp;
        getline(iss, msg);
        // 去掉消息前面的空格
        if(!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
    }
    // 验证合法性
    validateUserInput(usr, grp, client_sock);

    {

        // 完成合法性验证，将用户信息添加到客户端列表
        clients[usr] = {client_sock, usr, grp, false, time(nullptr)};
        // group_members[grp].insert(usr);
        // 如果群组没有拥有者，则第一个加入者将成为拥有者
        if (group_owners.count(grp) == 0) group_owners[grp] = usr;
    }
    
    // 如果该用户合法，那么：
    // 给group群组中的全部客户端广播加入信息
    broadcast(usr + " joined the group.");
    // 给加入的那个客户端单独发送信息
    sendToClient(client_sock, "You joined group [" + grp + "] as " + usr + (group_owners[grp] == usr ? " (owner)." : "."));


    /*#################核心指令处理逻辑#################*/
    while (true) {handleUserInput(usr, grp, msg);}

    {
        lock_guard<mutex> lock(client_mutex);
        
        // 断开连接时，删除客户端
        clients.erase(usr);
    }
    closesocket(client_sock);
}

int main() {
    /*#################初始化#################*/
    // 初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 打开chatlog文件
    chatlog.open("chatlog.txt", ios::app);
    if (!chatlog.is_open()) {
        cerr << "Failed to open chat log file!" << endl;
        return -1;
    }

    // 创建服务器套接字,并在端口监听
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr = { AF_INET, htons(8888), INADDR_ANY };
    bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    cout << "Server started on port 8888..." << endl;
    cout << "Chat history will be saved to chatlog.txt" << endl;

    // 把checkInactiveUsers函数扔到后台线程
    thread(checkInactiveUsers).detach();

    /*#################主循环#################*/
    while (true) {
        // 客户端发起连接请求的时候，这个进程会结束阻塞
        // 创建新的线程来处理客户端连接
        SOCKET client = accept(server_sock, nullptr, nullptr);
        // 拉起一个新的handleClient进程
        // 把刚创建的client对象作为参数传入
        thread(handleClient, client).detach();
    }

    /*#################服务器关闭流程#################*/
    chatlog.close();
    closesocket(server_sock);
    WSACleanup();
    return 0;
}