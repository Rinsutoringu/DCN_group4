
// #include "server.cpp"
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
    HANDLE_COMMAND("/history", handleShowHistory);
    HANDLE_COMMAND("/groupuser", handleShowGroupUser);
    HANDLE_COMMAND("/mute", handleMuteUser, usr);
    HANDLE_COMMAND("/unmute", handleUnmuteUser, usr);
    HANDLE_COMMAND("/kick", handleKickGroup, usr);
    HANDLE_COMMAND("/dismiss", handleDismissGroup, usr);
    HANDLE_COMMAND("/quit", handleQuit, usr);
    HANDLE_COMMAND("/help", handleHelp);

    // 如果用户的发言未匹配上方指令，那么按照发言处理
    broadcast(usr + ": " + msg);
}

// 创建群组
void handleCreateGroup(const string& group, const string& usr) {

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

// 加入群聊
void handleJoinGroup(const string& group, const string& usr) {

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 将该用户结构体中in_group置true
    sendToClient(client->socket, "You joined the group " + client->group);
    client->group = group;
}

// 你太菜了，退群吧
void handleLeaveGroup(const string& usr) {
    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    // 将该用户结构体中群名清空
    sendToClient(client->socket, "You left the group " + client->group);
    client->group = "";
}

// 展示所有连在服务器上的客户端
void handleShowAllClient() {

    for (const auto& pair : clients) {
        const string& username = pair.first;
        const ClientInfo& client = pair.second;
        sendToClient(client.socket, "User: " + username + ", Group: " + client.group);
    }
}

// 当前状态
void handleShowUserStatus(const string& usr) {

    // 从clients获取当前连接的用户
    lock_guard<mutex> lock(client_mutex);
    ClientInfo* client = getClient(usr);
    if (!client) return;

    sendToClient(client->socket, "User: " + usr + ", Group: " + client->group);
}

void handleShowHistory() {

}

void handleShowGroupUser() {

}

void handleMuteUser(const string& usr) {

}

void handleUnmuteUser(const string& usr) {

}

void handleKickGroup(const string& usr) {

}

void handleDismissGroup(const string& usr) {

}

void handleQuit(const string& usr) {

}

void handlehelp() {

}

void checkInactiveUsers() {
    
}