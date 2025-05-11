
// #include "server.cpp"
#include "server.h"

// 使用std作为命名空间
using namespace std;

// 建群
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

// 飞人
void handleKickGroup(const std::string& group, const std::string& username) {
    // 把用户从群组中移除，如果群组只有用户一个人，那么解散群聊
    if (!is_Owner(username)) {
        printf("用户 %s 不是群主，无法踢人", username.c_str());
        return;
    }
    auto it = group_members.find(group);
    if (it == group_members.end()) {
        printf("方法在group_members容器中未获得group对应值"); 
        return;
    }
    it->second.erase(username);
    broadcast(group, username + " left the group.");
    printf("用户 %s 离开了群组 %s", username.c_str(), group.c_str());
    
    if (!it->second.empty()) return;
    group_members.erase(it);
    group_owners.erase(group);
    printf("群组 %s 已解散", group.c_str());
}

// 塞口球
void handleMuteUser(const std::string& username) {
    // 获取用户，设置静音
    // TODO 客户端前端判定，当检测到被禁言时禁止用户输入
    lock_guard<mutex> lock(client_mutex);
    if (clients.count(username)) {
        clients[username].muted = true;
        sendToClient(clients[username].socket, "You have been muted.");
    }
}
// 拔口球
void handleUnmuteUser(const std::string& username) {
    lock_guard<mutex> lock(client_mutex);
    if (clients.count(username)) {
        clients[username].muted = false;
        sendToClient(clients[username].socket, "You have been unmuted.");
    }
}

