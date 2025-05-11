#include "server.h"

// 使用std作为命名空间
using namespace std;

// 处理用户输入
void handleUserInput(const string& usr, const string& grp, const string& msg) {
    lock_guard<mutex> lock(client_mutex);
    // 传入用户名和群名
    // TODO 进行合法性查验

    // 根据用户的输入调用不同函数
    // 如: /kick
    // 使用宏定义来自由化处理逻辑
    HANDLE_COMMAND("/create", handleCreateGroup, grp, usr);
    HANDLE_COMMAND("/join", handleJoinGroup, grp, usr);
    HANDLE_COMMAND("/leave", handleLeaveGroup, usr);
    HANDLE_COMMAND("/list_all_users", handleShowAllClient);
    HANDLE_COMMAND("/userstatus", handleShowUserStatus, usr);
    HANDLE_COMMAND("/history", handleShowHistory, chatlog, usr);
    HANDLE_COMMAND("/groupuser", handleShowGroupUser);
    HANDLE_COMMAND("/mute", handleMuteUser, usr);
    HANDLE_COMMAND("/unmute", handleUnmuteUser, usr);
    HANDLE_COMMAND("/kick", handleKickGroup, grp, usr);
    HANDLE_COMMAND("/dismiss", handleDismissGroup, usr);
    HANDLE_COMMAND("/quit", handleQuit, usr);
    HANDLE_COMMAND("/help", handleHelp, usr);

    // 如果用户的发言未匹配上方指令，那么按照发言处理
    sendToGroup(grp, usr + ": " + msg);
    // 存入chatlog文件
    chatlog << getTimestamp() + " " + usr + ": " + msg << endl;
    // 文件操作错误处理
    if (chatlog.fail()) cerr << "Error writing to chatlog file." << endl;
}


void handleCreateGroup(const string& group, const string& usr) {
    // 创建群组

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 已经有群了的用户不可以再创新的群
    if (client->group != "") {
        sendToClient(client->socket, "You are already in a group.");
        return;
    }
    // 将该用户结构体中in_group置true
    sendToClient(client->socket,"A group named " + group + " created successfully!");
    client->group = group;

    // 配置groupOwner
    group_owners[group] = usr;
}


void handleJoinGroup(const string& group, const string& usr) {
    // 加入群聊

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 将该用户结构体中in_group置true
    sendToClient(client->socket, "You joined the group " + client->group);
    client->group = group;
}


void handleLeaveGroup(const string& usr) {
    // 你太菜了，退群吧

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 将该用户结构体中群名清空
    sendToClient(client->socket, "You left the group " + client->group);
    client->group = "";
}


void handleShowAllClient() {
    // 展示所有连在服务器上的客户端

    for (const auto& pair : clients) {
        const string& username = pair.first;
        const ClientInfo& client = pair.second;
        sendToClient(client.socket, "User: " + username + ", Group: " + client.group);
    }
}


void handleShowUserStatus(const string& usr) {
    // 当前状态

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    sendToClient(client->socket, "User: " + usr + ", Group: " + client->group);
}

void handleShowHistory(ofstream& chatlog, const string& usr) {
    // 从已打开的chatlog对象获取最后20行，不足20行有多少就输出多少
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 在打开文件前，先关闭已有对象
    if (chatlog.is_open()) chatlog.close();

    // 使用 ifstream 打开文件以读取内容
    std::ifstream chatlog_in("chatlog.txt", std::ios::in);
    if (!chatlog_in.is_open()) {
        sendToClient(client->socket, "Error: Unable to open chat log file.");
        return;
    }

    std::deque<std::string> lines;
    std::string line;

    // 从文件头开始读取
    while (std::getline(chatlog_in, line)) {
        lines.push_back(line);
        if (lines.size() > 20) lines.pop_front();
    }
    // 发给客户端
    sendToClient(client->socket, "Chat history for user " + usr + ":");
    for (const auto& line : lines) {
        sendToClient(client->socket, line);
    }
}

void handleShowGroupUser() {
    // 展示群组内所有用户
    for (const auto& pair : clients) {
        const string& username = pair.first;
        const ClientInfo& client = pair.second;
        if (client.group == "") continue;
        sendToClient(client.socket, "User: " + username + ", Group: " + client.group);
    }
}

void handleMuteUser(const string& usr) {
    // 禁言用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    client->muted = true;
    sendToClient(client->socket, "You have been muted.");
}

void handleUnmuteUser(const string& usr) {
    // 解禁用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    client->muted = false;
    sendToClient(client->socket, "You have been unmuted.");
}

void handleKickGroup(const string& group, const string& usr) {
    // 踢出用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    if (client->group != group) {
        sendToGroup(client->group, "User " + usr + " is not in the group " + group + ".");
        return;
    }
    sendToGroup(group, "User " + usr + " has been kicked from the group.");
    client->group = "";
}

void handleDismissGroup(const string& usr) {
    // 解散群组
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 检查有没有在群里
    string group = client->group;
    if (group == "") {
        sendToClient(client->socket, "You are not in a group.");
        return;
    }
    // 检查这个人是不是群组拥有者
    if (!is_Owner(usr)) {
        sendToClient(client->socket, "You are not the owner of the group.");
        return;
    }

    // 在已注册的群组中查找该群组
    if (group_owners.find(group) == group_owners.end()) {
        cerr << "Group " << group << " does not exist." << endl;
        return;
    }
    // 发送解散群组的消息
    sendToGroup(group, "Group " + group + " has been dismissed.");
    // 移除群组的拥有者
    group_owners.erase(group);
    // 移除所有成员
    for (auto it = clients.begin(); it != clients.end();) {
        if (it->second.group == group) it->second.group = "";
        else ++it;
    }
    broadcast("Group " + group + " has been dismissed.");
}

void handleQuit(const string& usr) {
    // 客户端发送退出命令
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    string group = client->group;
    if (group != "") {
        sendToGroup(group, "User " + usr + " has left the group " + group + ".");
    }
    clients.erase(usr);
}

void handleHelp(const string& usr) {
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    sendToClient(client->socket, "Available commands:");
    sendToClient(client->socket, "/history - Show chat history");
    sendToClient(client->socket, "/kick <user> - Kick a user from the group");
    sendToClient(client->socket, "/mute <user> - Mute a user");
    sendToClient(client->socket, "/unmute <user> - Unmute a user");
    sendToClient(client->socket, "/create <group> - Create a new group");
    sendToClient(client->socket, "/join <group> - Join an existing group");
    sendToClient(client->socket, "/leave - Leave the current group");
    sendToClient(client->socket, "/help - Show this help message");
}


void checkInactiveUsers() {
    // 踢出超时用户

    lock_guard<mutex> lock(client_mutex);
    for (auto it = clients.begin(); it != clients.end();) {
        if (it->second.last_activity < time(nullptr) - INACTIVITY_TIMEOUT) {
            sendToClient(it->second.socket, "You have been inactive for too long. You will be disconnected.");
            it = clients.erase(it);
        } else ++it;
    }
}