#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "server.h"
#pragma comment(lib, "ws2_32.lib")

// 定义全局变量
std::map<std::string, std::set<std::string>> group_members;
std::map<std::string, std::string> group_owners;
std::map<std::string, ClientInfo> clients;
std::mutex client_mutex;
std::ofstream chatlog;

// 使用std作用域
using namespace std;

string xorCipher(const string& data) {
    string res = data;
    for (char& c : res) c ^= XOR_KEY;
    return res;
}


string getTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm tm; localtime_s(&tm, &t);
    ostringstream oss;
    oss << put_time(&tm, "[%H:%M]");
    return oss.str();
}

void sendToClient(SOCKET sock, const string& msg) {
    // 执行信息加密
    string encrypted = xorCipher(msg);
    send(sock, encrypted.c_str(), encrypted.size(), 0);
}


void broadcast(const string& group, const string& msg, const string& except) {
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


void handleClient(SOCKET client_sock) {
    string usr, grp, msg;
    char buffer[1024];

    // 获取客户端报文并进行处理
    {
        lock_guard<mutex> lock(client_mutex);
        
        int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) return;
        // 解密
        istringstream iss(xorCipher(std::string(buffer, len)));
        // 流式切割报文
        
        iss >> usr >> grp;
        getline(iss, msg);
        // 去掉消息前面的空格
        if(!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
    }

    // 合法性验证：检查用户名和群组名是否为空
    {
        lock_guard<mutex> lock(client_mutex);
        if (usr.empty() || grp.empty())
        {
            sendToClient(client_sock, "Error: Username and group cannot be empty.");
            closesocket(client_sock);
            return;
        }
    }

    // 合法性验证：检查用户名是否已存在
    {
        lock_guard<mutex> lock(client_mutex);
        if (clients.count(usr)) {
            sendToClient(client_sock, "Error: Username already in use.");
            closesocket(client_sock);
            return;
        }
        // 完成合法性验证，将用户信息添加到客户端列表
        clients[usr] = {client_sock, usr, grp, false, time(nullptr)};
        group_members[grp].insert(usr);
        // 如果群组没有拥有者，则第一个加入者将成为拥有者
        if (group_owners.count(grp) == 0) group_owners[grp] = usr;
    }
    
    // 如果该用户合法，那么：
    // 给group群组中的全部客户端广播加入信息
    broadcast(grp, usr + " joined the group.", usr);
    // 给加入的那个客户端单独发送信息
    sendToClient(client_sock, "You joined group [" + grp + "] as " + usr + (group_owners[grp] == usr ? " (owner)." : "."));

    // 死循环 不断获取用户输入内容，判断该如何处理
    while (true) {
        
        // 获取消息长度
        int msg_len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (msg_len <= 0) break;

        // 获取用户最后在线时间
        {
            lock_guard<mutex> lock(client_mutex);
            clients[usr].last_activity = time(nullptr);
        }
        // TODO 昨晚干到这里

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
        // 也是对指令的处理
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
            // 如果不是指令，那么一定是用户说话了
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
    /*#################初始化#################*/
    // 初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 打开chatlog文件
    chatlog.open("chatlog.txt", ios::app);
    if (!chatlog.is_open()) {
        cerr << "Failed to open chat log file!" << endl;
        return -1;
    }

    // 创建服务器套接字,并在端口监听
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr = { AF_INET, htons(8888), INADDR_ANY };
    bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    cout << "Server started on port 8888..." << endl;
    cout << "Chat history will be saved to chatlog.txt" << endl;

    // 把checkInactiveUsers函数扔到后台线程
    thread(checkInactiveUsers).detach();

    /*#################主循环#################*/
    while (true) {
        // 客户端发起连接请求的时候，这个进程会结束阻塞
        // 创建新的线程来处理客户端连接
        SOCKET client = accept(server_sock, nullptr, nullptr);
        // 拉起一个新的handleClient进程
        // 把刚创建的client对象作为参数传入
        thread(handleClient, client).detach();
    }

    /*#################服务器关闭流程#################*/
    chatlog.close();
    closesocket(server_sock);
    WSACleanup();
    return 0;
}