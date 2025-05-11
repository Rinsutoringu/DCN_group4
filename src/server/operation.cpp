
// #include "server.cpp"
#include "server.h"

// 使用std作为命名空间
using namespace std;

void handleCreateGroup(const std::string& group, const std::string& username) {
    // // 先判断用户是否是管理员
    // if (!is_Admin(username)) {
    //     printf("用户 %s 不是管理员，无法创建群组", username.c_str());
    //     return;
    // }

    // 创建群组
    group_members[group] = std::set<std::string>();
    group_owners[group] = username;
    printf("用户 %s 创建了群组 %s", username.c_str(), group.c_str());
}

void handleLeaveGroup(const std::string& group, const std::string& username) {
    // 先获取群组成员列表
    
    // 使用find方法，从group_members容器中查找group键对应的值
    auto it = group_members.find(group);
    // 判断获得的it是否是有效的（it可能获取到了group_members.end()）
    if (it == group_members.end()) {
        printf("方法在group_members容器中未获得group对应值"); 
        return;
    }
    it->second.erase(username);
    // 判断列表是否为空
    if (!it->second.empty()) return;
    group_members.erase(it);
    group_owners.erase(group);
    // 把用户移除出了群组
    broadcast(group, username + " left the group.");
}

void handleMuteUser(const std::string& username) {
    // 获取用户，设置静音
    // TODO 客户端前端判定，当检测到被禁言时禁止用户输入
    lock_guard<mutex> lock(client_mutex);
    if (clients.count(username)) {
        clients[username].muted = true;
        sendToClient(clients[username].socket, "You have been muted.");
    }
}

void handleUnmuteUser(const std::string& username) {
    lock_guard<mutex> lock(client_mutex);
    if (clients.count(username)) {
        clients[username].muted = false;
        sendToClient(clients[username].socket, "You have been unmuted.");
    }
}