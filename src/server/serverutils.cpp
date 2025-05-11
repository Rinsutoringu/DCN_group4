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
    for (const auto& group : group_owners) {
        if (group.second == username) {
            return true;
        }
    }
    return false;
}