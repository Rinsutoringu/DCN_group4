#include "server.h"

/**
 * 判断用户是否是管理员
 * 暂时不用
 * @param username 用户名
 * @return 如果是管理员返回true，否则返回false
 */
bool is_Admin(const std::string& username) {
    // 判断用户是否是管理员
    auto it = clients.find(username);
    if (it != clients.end()) {
        return it->second.username == "admin";
    }
    return false;
}

/**
 * 判断用户是否是群组持有者
 * @param username 用户名
 * @return 如果是群主返回true，否则返回false
 */
bool is_Owner(const std::string& username) {
    // 判断用户是否是群组持有者
    for (const auto& group : group_owners) {
        if (group.second == username) {
            return true;
        }
    }
    return false;
}