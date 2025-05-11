
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
    HANDLE_COMMAND("/join", handleJoinGroup, usr);
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
    broadcast(grp, usr + ": " + msg);
}

void handleCreateGroup(const string& group, const string& username) {

    // 创建群组
    lock_guard<mutex> lock(client_mutex);
    if (group_members.find(group) == group_members.end()) {
        group_members[group] = set<string>();
        group_owners[group] = username;
    }
}

void handleJoinGroup(const string& username) {
    // 获取用户
    auto it = clients.find(username);
    if (it == clients.end()) {
        cerr << "User " << username << " not found." << endl;
        return;
    }
    ClientInfo& client = it->second;
    if (client.group.empty()) {
        cerr << "User " << username << " is not in a group." << endl;
        return;
    }
    group_members[client.group].insert(client.username);
}

void handleLeaveGroup(const string& username) {

}

void handleShowAllClient() {

}

void handleShowUserStatus(const string& username) {

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