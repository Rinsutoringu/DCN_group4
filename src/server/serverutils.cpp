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
        return;
    }
    if (clients.count(usr)) {
        sendToClient(client_sock, "Error: Username already in use.");
        closesocket(client_sock);
        return;
    }
    return;
}

// 私聊
void sendToClient(SOCKET sock, const string& msg) {
    // 执行信息加密
    
    // 为消息附加调试信息
    string debug_info = getTimestamp() + " 服务器私聊模块发送的 " + msg;
    string encrypted = xorCipher(debug_info);
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
void sendToGroup(const std::string& usr, const std::string& group, const std::string& msg) {
    // 去除空格和不可见字符
    string trimmed_msg = msg;
    trimmed_msg.erase(remove_if(trimmed_msg.begin(), trimmed_msg.end(), ::isspace), trimmed_msg.end());

    // 判断是否为空
    if (trimmed_msg.empty()) return;

    // 判断是否被禁言
    ClientInfo* client = getClient(usr);
    int muteStatus = muteCheck(client->username);
    if (muteStatus != 0) return;

    // 打印到日志
    if (msg[0] == '/') return;
    chatlog << getTimestamp() + " " + usr + ": " + msg << endl;
    if (chatlog.fail()) cerr << "Error writing to chatlog file." << endl;
    cerr << "chatlog写入完成" << endl;

    // 拼接字符串
    string timestamped_msg = getTimestamp() + " " + msg;
    for (const auto& [username, client] : clients) {
        if (client.group == group) sendToClient(client.socket, timestamped_msg);
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
    // DEBUG

    // DEBUG
    cerr << "检查用户禁言状态: 获取用户" << endl;
    ClientInfo* client = getClient(usr);
    if (!client) return -1; // 没找到套接字返回-1
    if (client->muted) return 1; // 被禁言了是true
    return 0;
}

bool getSocketMessage(SOCKET client_sock, string& usr, string& grp, string& msg) {
    char buffer[1024];
    int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) return false; // 客户端断开连接或发生错误

    // 解密并解析消息
    istringstream iss(xorCipher(std::string(buffer, len)));
    iss >> usr >> grp; // 流式切割报文
    getline(iss, msg);

    // 去掉消息前面的空格
    if (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);

    return true;
}