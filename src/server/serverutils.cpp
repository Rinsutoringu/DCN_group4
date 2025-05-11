#include "server.h"
// 使用std作为命名空间
using namespace std;


bool is_Owner(const std::string& username) {
    // 判断用户是否是群组持有者
    ClientInfo* client = getClient(username);
    if (!client) return false;


    string group = client->group;
    // 检查有没有在任何群里
    if (group == "") return false;
    // 检查群组拥有者
    return group_owners[group] == username;
}

// 验证用户输入合法性
void validateUserInput(const string& usr, const string& grp, SOCKET client_sock) {

    if (usr.empty() || grp.empty())
    {
        sendToClient(client_sock, "Error: Username and group cannot be empty.");
        closesocket(client_sock);
    }
    if (clients.count(usr)) {
        sendToClient(client_sock, "Error: Username already in use.");
        closesocket(client_sock);
    }
    return;
}

// 私聊
void sendToClient(SOCKET sock, const string& msg) {
    // 执行信息加密
    string encrypted = xorCipher(msg);
    send(sock, encrypted.c_str(), encrypted.size(), 0);
}

// 广播
void broadcast(const string& msg) {
    // 进程锁

    // 拼接字符串
    string timestamped_msg = getTimestamp() + " " + msg;
    // 所有在服务器的客户端都应接收到广播
    for (const auto& [username, client] : clients) {
        sendToClient(client.socket, timestamped_msg);
    }
}

// 正常的群聊内部广播
void sendToGroup(const std::string& group, const std::string& msg) {

    // 拼接字符串
    string timestamped_msg = getTimestamp() + " " + msg;
    // 所有在群组中的客户端都应接收到广播
    for (const auto& [username, client] : clients) {
        if (client.group == group) {
            sendToClient(client.socket, timestamped_msg);
        }
    }
}

ClientInfo* getClient(const string& usr) {
    auto it = clients.find(usr);
    if (it == clients.end()) {
        cerr << "User " << usr << " not found." << endl;
        return nullptr; // Return nullptr if user not found
    }
    ClientInfo& client = it->second;
    return &client;
}

int muteCheck(const std::string& usr) {

    cerr << "检查用户禁言状态: 获取用户" << endl;
    ClientInfo* client = getClient(usr);
    if (!client) return -1; // 没找到套接字返回-1
    if (client->muted) return 1; // 被禁言了是true
    return 0;
}

void userInputThread(SOCKET client_sock, string& msg) {
    // DEBUG
    cerr << "用户输入线程启动成功" << endl;
    char buffer[1024]; // 用于接收客户端报文
    while (true) {
        // 从客户端接收消息
        int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            // 如果接收失败或客户端断开连接，退出线程
            cerr << "Client disconnected or recv failed." << endl;
            return;
        }
        // 解密报文
        istringstream iss(xorCipher(std::string(buffer, len)));
        string decrypted = iss.str();
        {
            // 将接收到的消息存储到共享变量 msg 中
            lock_guard<mutex> lock(input_mutex);
            msg = decrypted;
        }
        // 通知主线程有新消息
        input_cv.notify_one();
    }
}