#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "server.h"
#pragma comment(lib, "ws2_32.lib")

// 使用std作用域
using namespace std;

// 定义全局变量
map<std::string, std::string> group_owners;
map<std::string, ClientInfo> clients;
mutex client_mutex;
ofstream chatlog;



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
    // DEBUG
    cerr << "客户端线程启动成功" << endl;
    string usr, grp, msg;
    char buffer[1024];
    // 获取客户端报文并进行处理
    {
        lock_guard<mutex> lock(client_mutex);
        int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) return;
        istringstream iss(xorCipher(std::string(buffer, len))); // 解密
        iss >> usr >> grp; // 流式切割报文
        getline(iss, msg);
        if(!msg.empty() && msg[0] == ' ') msg = msg.substr(1); // 去掉消息前面的空格
    }
    // DEBUG
    cerr << "客户端报文处理完成" << endl;

    validateUserInput(usr, grp, client_sock); // 验证合法性 这真的不是GPT写的，这是我手敲的

    // DEBUG
    cerr << "合法性验证完成" << endl;
    {
        // 完成合法性验证，将用户信息添加到客户端列表
        clients[usr] = {client_sock, usr, grp, false, time(nullptr)};
        // 如果群组没有拥有者，则第一个加入者将成为拥有者
        if (group_owners.count(grp) == 0) group_owners[grp] = usr;
    }
    // DEBUG
    cerr << "客户端列表添加完成" << endl;
    sendToGroup(grp, usr + " joined the group.");
    sendToClient(client_sock, "You joined group [" + grp + "] as " + usr + (group_owners[grp] == usr ? " (owner)." : "."));
    // DEBUG
    cerr << "已发送连接通告" << endl;

    /*#################核心指令处理逻辑#################*/
    while (true) {
        handleUserInput(usr, grp, msg);
        sendToClient(client_sock, "You sent a message: " + msg);
    }

    // 退出处置
    {
        lock_guard<mutex> lock(client_mutex);
        clients.erase(usr);
    }
    closesocket(client_sock);
}

int main() {
    /*#################初始化#################*/
    // 初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 打开chatlog文件，支持读写和追加模式
    // DEBUG
    cerr << "准备打开chatlog文件" << endl;
    chatlog.open("chatlog.txt", ios::in | ios::out | ios::app);
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
    // DEBUG
    cerr << "启动守护进程...." << endl;
    thread(checkInactiveUsers).detach();

    /*#################主循环#################*/
    while (true) {
        // DEBUG
        cerr << "等待客户端连接..." << endl;
        // 客户端发起连接请求的时候，这个进程会结束阻塞
        // 创建新的线程来处理客户端连接
        SOCKET client = accept(server_sock, nullptr, nullptr);
        // 拉起一个新的handleClient进程
        // 把刚创建的client对象作为参数传入
        // DEBUG
        cerr << "启动新的客户端处理线程..." << endl;
        thread(handleClient, client).detach();
    }

    /*#################服务器关闭流程#################*/
    chatlog.close();
    closesocket(server_sock);
    WSACleanup();
    return 0;
}