#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "server.h"

// 定义全局变量
std::map<std::string, std::set<std::string>> group_members;
std::map<std::string, std::string> group_owners;
std::map<std::string, ClientInfo> clients;
std::mutex client_mutex;
std::ofstream chatlog;


#pragma comment(lib, "ws2_32.lib")
using namespace std;

// XOR加密函数
string xorCipher(const string& data) {
    string res = data;
    for (char& c : res) c ^= XOR_KEY;
    return res;
}

/**
 * 获取当前时间戳
 * @return 当前时间戳字符串
 */
string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm tm; localtime_s(&tm, &t);
    ostringstream oss;
    oss << put_time(&tm, "[%H:%M]");
    return oss.str();
}

/**
 * 发送信息到客户端
 * @param sock 客户端socket
 * @param msg 发送的消息
 */
void sendToClient(SOCKET sock, const string& msg) {
    string encrypted = xorCipher(msg);
    send(sock, encrypted.c_str(), encrypted.size(), 0);
}

/**
 * 指定群组，广播消息到所有在该群组中的客户端
 * @param group 群组名称
 * @param msg 发送的消息
 * @param except 排除的用户
 */
void broadcast(const string& group, const string& msg, const string& except = "") {
    // 进程锁
    lock_guard<mutex> lock(client_mutex);
    if (group_members.find(group) == group_members.end()) return;
    // msg字符串
    string timestamped_msg = getTimestamp() + " " + msg;
    chatlog << timestamped_msg << endl;
    
    for (const auto& username : group_members[group]) {
        if (username == except) continue;
        sendToClient(clients[username].socket, timestamped_msg);
    }
}

/**
 * 检查不活跃的用户
 */
void checkInactiveUsers() {

    while (true) {
        this_thread::sleep_for(chrono::seconds(30));

        lock_guard<mutex> lock(client_mutex);
        time_t now = time(nullptr);
        // 待移除用户
        vector<string> to_remove;
        
        for (auto &[username, client] : clients)
        {
            if (!difftime(now, client.last_activity) > INACTIVITY_TIMEOUT) continue;

            cout << "User " << username << " timed out due to inactivity." << endl;
            to_remove.push_back(username);
            closesocket(client.socket);
        }

        for (const auto &username : to_remove)
        {
            string group = clients[username].group;
            // 如果用户未加入任何群组，则跳过
            if (group.empty()) continue;
            group_members[group].erase(username);
            
            // 如果群组为空，则删除群组
            if (!group_members[group].empty()) continue;
            group_members.erase(group);
            group_owners.erase(group);
            clients.erase(username);
        }
    }
}

/**
 * 处理客户端连接
 * @param client_sock 客户端socket
 */
void handleClient(SOCKET client_sock) {
    char buffer[1024];

    int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) return;
    for (int i = 0; i < len; ++i) buffer[i] ^= XOR_KEY;
    buffer[len] = '\0';

    string username = strtok(buffer, " ");
    string group = strtok(NULL, " ");
    // 合法性验证：如果用户名和群组名为空，则关闭连接
    {
        lock_guard<mutex> lock(client_mutex);
        if (username.empty() || group.empty())
        {
            sendToClient(client_sock, "Error: Username and group cannot be empty.");
            closesocket(client_sock);
            return;
        }
    }

    // 合法性验证：检查用户名是否已存在
    {
        lock_guard<mutex> lock(client_mutex);
        if (clients.count(username)) {
            sendToClient(client_sock, "Error: Username already in use.");
            closesocket(client_sock);
            return;
        }
        clients[username] = {client_sock, username, group, false, time(nullptr)};
        group_members[group].insert(username);
        if (group_owners.count(group) == 0)
            group_owners[group] = username;
    }
    
    // 如果该用户合法，那么：
    // 给group群组中的全部客户端广播加入信息
    broadcast(group, 
        username + " joined the group.", username);
    // 给加入的那个客户端单独发送信息
    sendToClient(client_sock,
        "You joined group [" + group + "] as " + username + (group_owners[group] == username ? " (owner)." : "."));

    while (true) {
        int msg_len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (msg_len <= 0) break;

        // 获取用户最后在线时间
        {
            lock_guard<mutex> lock(client_mutex);
            clients[username].last_activity = time(nullptr);
        }
        // TODO 格式化到这里为止

        for (int i = 0; i < msg_len; ++i) buffer[i] ^= XOR_KEY;
        buffer[msg_len] = '\0';
        string msg = buffer;

        // 对于指令的处理
        {
            lock_guard<mutex> lock(client_mutex);
            if (clients[username].muted) {
                if (msg == "/quit" || msg == "/leave") {
                    if (msg == "/leave") {
                        broadcast(group, username + " left the group.");
                        group_members[group].erase(username);
                        clients[username].group = "";

                        if (group_members[group].empty()) {
                            group_members.erase(group);
                            group_owners.erase(group);
                        }
                    }
                    break;
                } else {
                    sendToClient(client_sock, "You are muted and cannot send messages.");
                    continue;
                }
            }

            if (clients[username].group.empty() || group_members[group].find(username) == group_members[group].end()) {
                sendToClient(client_sock, "You are not in the group and cannot send messages.");
                continue;
            }
        }

        if (msg == "/quit") {
            break;
        } else if (msg == "/leave") {
            lock_guard<mutex> lock(client_mutex);
            broadcast(group, username + " left the group.");
            group_members[group].erase(username);
            clients[username].group = "";

            if (group_members[group].empty()) {
                group_members.erase(group);
                group_owners.erase(group);
            }
            break;        
        } else if (msg.rfind("/mute ", 0) == 0) {
            string target = msg.substr(6);
            if (group_owners[group] != username) {
                sendToClient(client_sock, "Only owner can mute.");
                continue;
            }
            {
                lock_guard<mutex> lock(client_mutex);
                if (clients.count(target)) {
                    clients[target].muted = true;
                    sendToClient(clients[target].socket, "You have been muted.");
                }
            }
            continue;
        } else if (msg.rfind("/unmute ", 0) == 0) {
            string target = msg.substr(8);
            if (group_owners[group] != username) {
                sendToClient(client_sock, "Only owner can unmute.");
                continue;
            }
            {
                lock_guard<mutex> lock(client_mutex);
                if (clients.count(target)) {
                    clients[target].muted = false;
                    sendToClient(clients[target].socket, "You have been unmuted.");
                }
            }
            continue;
        } else if (msg.rfind("/kick ", 0) == 0) {
            string target = msg.substr(6);
            if (group_owners[group] != username) {
                sendToClient(client_sock, "Only owner can kick."); 
                continue;
            }
            {
                lock_guard<mutex> lock(client_mutex);
                if (clients.count(target)) {
                    sendToClient(clients[target].socket, "CMD_KICK:You have been kicked from group [" + group + "].");
                    group_members[group].erase(target);
                    clients[target].group = "";
                }
            }
            continue;
        } else if (msg.rfind("/groupuser ", 0) == 0) {
            string target = msg.substr(11);
            if (group_owners[group] != username) {
                sendToClient(client_sock, "Only owner can add users to the group.");
                continue;
            }
            {
                lock_guard<mutex> lock(client_mutex);
                if (clients.count(target)) {
                    if (clients[target].group.empty()) {
                        clients[target].group = group;
                        group_members[group].insert(target);
                        sendToClient(clients[target].socket, "You have been added to group [" + group + "] by the owner.");
                        broadcast(group, target + " has been added to the group by the owner.", target);
                    } else {
                        sendToClient(client_sock, "User is already in a group.");
                    }
                } else {
                    sendToClient(client_sock, "User not found.");
                }
            }
            continue;
        } else if (msg == "/dismiss") {
            if (group_owners[group] != username) {
                sendToClient(client_sock, "Only owner can dismiss group."); continue;
            }
            broadcast(group, "Group dismissed by owner.");
            for (auto& u : group_members[group])
                if (u != username) closesocket(clients[u].socket);
            {
                lock_guard<mutex> lock(client_mutex);
                for (auto& u : group_members[group]) clients.erase(u);
                group_members.erase(group);
                group_owners.erase(group);
            }
            break;
        } else if (msg == "/list_all_users") {
            string userList = "All users and their groups:\n";
            {
                lock_guard<mutex> lock(client_mutex);
                for (const auto& client : clients) {
                    userList += client.first + " (Group: " + 
                               (client.second.group.empty() ? "None" : client.second.group) + ")\n";
                }
            }
            sendToClient(client_sock, userList);
            continue;
        } else if (msg.rfind("/userstatus ", 0) == 0) {
            string target = msg.substr(12);
            {
                lock_guard<mutex> lock(client_mutex);
                if (clients.count(target)) {
                    time_t now = time(nullptr);
                    string status = (difftime(now, clients[target].last_activity) < INACTIVITY_TIMEOUT) 
                        ? "Online" : "Offline";
                    sendToClient(client_sock, "User " + target + " is currently: " + status);
                } else {
                    sendToClient(client_sock, "User " + target + " not found or offline.");
                }
            }
            continue;
        } else if (msg == "/history") {
            ifstream logfile("chatlog.txt");
            string history, line;
            while (getline(logfile, line)) {
                history += line + "\n";
            }
            sendToClient(client_sock, "Last 20 messages:\n" + 
                (history.empty() ? "No history yet" : history));
            continue;
        } else {
            broadcast(group, username + ": " + msg, username);
        }
    }

    {
        lock_guard<mutex> lock(client_mutex);
        group_members[group].erase(username);
        clients.erase(username);
    }
    closesocket(client_sock);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Open chat log file
    chatlog.open("chatlog.txt", ios::app);
    if (!chatlog.is_open()) {
        cerr << "Failed to open chat log file!" << endl;
        return -1;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr = { AF_INET, htons(8888), INADDR_ANY };
    bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    cout << "Server started on port 8888..." << endl;
    cout << "Chat history will be saved to chatlog.txt" << endl;

    thread(checkInactiveUsers).detach();

    while (true) {
        SOCKET client = accept(server_sock, nullptr, nullptr);
        thread(handleClient, client).detach();
    }

    chatlog.close();
    closesocket(server_sock);
    WSACleanup();
    return 0;
}