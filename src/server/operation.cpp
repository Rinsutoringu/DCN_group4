
// #include "server.cpp"
#include "server.h"

// 使用std作为命名空间
using namespace std;

// 处理用户输入
void handleUserInput(string msg) {
    // 传入用户名和群名
    // TODO 进行合法性查验
    
    // 根据用户的输入调用不同函数
    // 如: /kick
    // 使用宏定义来自由化处理逻辑
    HANDLE_COMMAND("/create", handleCreateGroup());
    HANDLE_COMMAND("/join", handleKickGroup());
    HANDLE_COMMAND("/leave", handleMuteUser());
    HANDLE_COMMAND("/list_all_users", handleUnmuteUser());
    HANDLE_COMMAND("/userstatus", handleDismissGroup());
    HANDLE_COMMAND("/history", handleDismissGroup());
    HANDLE_COMMAND("/groupuser", handleDismissGroup());
    HANDLE_COMMAND("/mute", handleDismissGroup());
    HANDLE_COMMAND("/unmute", handleDismissGroup());
    HANDLE_COMMAND("/kick", handleDismissGroup());
    HANDLE_COMMAND("/dismiss", handleDismissGroup());
    HANDLE_COMMAND("/quit", handleDismissGroup());
    HANDLE_COMMAND("/help", handleHelp());
}

