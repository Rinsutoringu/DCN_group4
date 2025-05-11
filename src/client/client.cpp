#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "client.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

// 构造函数
ChatClient::ChatClient(const char* server_name, unsigned short port) {
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_name);

    connect_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_sock == INVALID_SOCKET) {
        lock_guard<mutex> lock(cout_mutex);
        cerr << "Socket creation failed!" << endl;
        WSACleanup();
        exit(-1);
    }

    if (connect(connect_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        lock_guard<mutex> lock(cout_mutex);
        cerr << "Connection failed!" << endl;
        WSACleanup();
        exit(-1);
    }
}

// 加密算法
string ChatClient::xorCipher(const string& data) {
    string res = data;
    for (auto& c : res) {
        c ^= XOR_KEY;
    }
    return res;
}

void ChatClient::sendMessage(const string& message) {
    // 向服务器发信

    string encrypted = xorCipher(message);// 进行加密
    // 发送，并进行错误处理
    if (send(connect_sock, encrypted.c_str(), encrypted.length(), 0) == SOCKET_ERROR) {
        lock_guard<mutex> lock(cout_mutex);
        cerr << "Send failed!" << endl;
    }
}

void ChatClient::receiveMessage() {
    cerr << "receivemessage上锁" << endl;
    lock_guard<mutex> lock(cout_mutex);
    cerr << "上锁成功" << endl;

    char buffer[1024];
    while (true) {

        // 接收消息，存入message
        int msg_len = recv(connect_sock, buffer, sizeof(buffer) - 1, 0);
        if (msg_len > 0) {
            for (int i = 0; i < msg_len; ++i) buffer[i] ^= XOR_KEY;
            buffer[msg_len] = '\0';
            string msg = buffer;
            cout << msg << endl;
            
            if (msg.find("CMD_KICKOUT:") != string::npos) cout << "You are kicked.\nEnter command (/create, /join): ";
            cout.flush();
        }
        else if (msg_len == 0) {
            cout << "\nDisconnected from server." << endl;
            exit(0);
        } else {
            cerr << "\nReceive failed!" << endl;
            exit(-1);
        }
    }
}

void ChatClient::handleConnection() {
    // 创建接收消息线程
    thread receiver(&ChatClient::receiveMessage, this);
    // 创建用户输入线程
    thread input_thread(&ChatClient::getMessage, this);

    
    while (true) {
        string input;
        {
            unique_lock<mutex> lock(input_mutex);
            input_cv.wait(lock, [&]() { return !input_queue.empty() || exit_flag; });

            if (exit_flag) {
                sendMessage("/quit");
                closesocket(connect_sock);
                WSACleanup();
                break;
            }

            input = input_queue.front();
            input_queue.pop();
        }

        // 发送消息到服务器
        sendMessage(input);
    }

    input_thread.join(); // 等待输入线程结束
}

void ChatClient::showHelp() {
    lock_guard<mutex> lock(cout_mutex);
    cout << "\n========= Chat Client Commands =========\n";
    cout << "/create <group_name>     - Create a new group\n";
    cout << "/join <group_name>       - Join an existing group\n";
    cout << "/leave                   - Leave the current group\n";
    cout << "/list_all_users          - List all users and their groups\n";
    cout << "/userstatus <username>   - Check if a user is online/offline\n";
    cout << "/history                 - View last 20 messages\n";
    cout << "/groupuser <username>    - Add user to group (owner only)\n";
    cout << "/mute <username>         - Mute user (owner only)\n";
    cout << "/unmute <username>       - Unmute user (owner only)\n";
    cout << "/kick <username>         - Kick user (owner only)\n";
    cout << "/dismiss                 - Dismiss group (owner only)\n";
    cout << "/quit                    - Quit the client\n";
    cout << "/help                    - Show this help message\n";
    cout << "Type a message to send it to the current group\n";
    cout << "========================================\n\n";
}

void ChatClient::start() {
    string group;

    // 展示帮助菜单（本地处理）
    showHelp();

    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Enter username: ";
    }
    getline(cin, username);

    while (true) {
        
        // 提示用户输入命令
        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Enter command (/create <group>, /join <group>): ";
        }
        // 获取用户命令
        string input;
        getline(cin, input);

        // 本地验证初始化命令合法性
        size_t space_pos = input.find(' ');
        if (space_pos == string::npos) {
            lock_guard<mutex> lock(cout_mutex);
            cerr << "Invalid command. Usage: /create <group> or /join <group>\n";
            continue;
        }
        string cmd = input.substr(0, space_pos);
        group = input.substr(space_pos + 1);
        if (cmd != "/create" && cmd != "/join") {
            lock_guard<mutex> lock(cout_mutex);
            cerr << "Invalid command. Usage: /create <group> or /join <group>\n";
            continue;
        }

        break;
    }
    sendMessage(username + " " + group);
    handleConnection();
}

void ChatClient::getMessage() {
    string input;
    while (true) {
        // 为输入消息创建锁
        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Enter a message: ";
        }
        getline(cin, input);

        // 处理退出指令
        {
            lock_guard<mutex> lock(input_mutex);
            if (input == "/quit") {
                exit_flag = true;
                input_cv.notify_all();
                break;
            }
            input_queue.push(input);
        }
        input_cv.notify_one(); // 通知主线程有新消息
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Usage: client <server IP> <port>" << endl;
        return -1;
    }

    const char* server_name = argv[1];
    unsigned short port = atoi(argv[2]);

    ChatClient client(server_name, port);
    client.start();

    return 0;
}