
// #include "server.cpp"
#include "server.h"

// 使用std作为命名空间
using namespace std;

// 处理用户输入
void handleUserInput(const std::string& usr, const std::string& grp, const std::string& msg) {
    lock_guard<mutex> lock(client_mutex);
    // 传入用户名和群名
    // TODO 进行合法性查验

    // 根据用户的输入调用不同函数
    // 如: /kick
    // 使用宏定义来自由化处理逻辑
    HANDLE_COMMAND("/create", handleCreateGroup, grp, usr);
    HANDLE_COMMAND("/join", handleJoinGroup, usr);
    HANDLE_COMMAND("/leave", handleMuteUser, usr);
    HANDLE_COMMAND("/list_all_users", handleUnmuteUser, usr);
    HANDLE_COMMAND("/userstatus", handleDismissGroup, usr);
    HANDLE_COMMAND("/history", handleDismissGroup, usr);
    HANDLE_COMMAND("/groupuser", handleDismissGroup, usr);
    HANDLE_COMMAND("/mute", handleDismissGroup, usr);
    HANDLE_COMMAND("/unmute", handleDismissGroup, usr);
    HANDLE_COMMAND("/kick", handleDismissGroup, usr);
    HANDLE_COMMAND("/dismiss", handleDismissGroup, usr);
    HANDLE_COMMAND("/quit", handleDismissGroup, usr);
    HANDLE_COMMAND("/help", handleHelp);
}

void handleCreateGroup(const std::string& group, const std::string& username) {

}

void handleJoinGroup(const std::string& username) {

}

void handleMuteUser(const std::string& username) {

}

void handleUnmuteUser(const std::string& username) {

}

void handleDismissGroup(const std::string& username) {

}

