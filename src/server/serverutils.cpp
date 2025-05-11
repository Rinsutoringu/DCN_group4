#include "server.h"
// 使用std作为命名空间
using namespace std;

bool is_Admin(const std::string& username) {
    // 判断用户是否是管理员
    auto it = clients.find(username);
    if (it != clients.end()) {
        return it->second.username == "admin";
    }
    return false;
}


bool is_Owner(const std::string& username) {
    // 判断用户是否是群组持有者
    // 因为其实最多只能创建一个群组，所以直接获取group_owners的第二个元素即可。
    auto it = group_owners.find(username);
    if (it != group_owners.end()) {
        return it->second == username;
    }
    return false;
}

// 验证用户输入合法性
void validateUserInput(const string& usr, const string& grp, SOCKET client_sock) {
    lock_guard<mutex> lock(client_mutex);
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