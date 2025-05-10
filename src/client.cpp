#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define XOR_KEY 0xAB

class ChatClient {
public:
    ChatClient(const char* server_name, unsigned short port);
    void start();

private:
    SOCKET connect_sock;
    struct sockaddr_in server_addr;
    WSADATA wsaData;
    mutex cout_mutex;
    string username;

    string xorCipher(const string& data);
    void sendMessage(const string& message);
    void receiveMessage();
    void handleConnection();
    void showHelp();
};

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

string ChatClient::xorCipher(const string& data) {
    string res = data;
    for (auto& c : res) {
        c ^= XOR_KEY;
    }
    return res;
}

void ChatClient::sendMessage(const string& message) {
    string encrypted = xorCipher(message);
    if (send(connect_sock, encrypted.c_str(), encrypted.length(), 0) == SOCKET_ERROR) {
        lock_guard<mutex> lock(cout_mutex);
        cerr << "Send failed!" << endl;
    }
}

void ChatClient::receiveMessage() {
    char buffer[1024];
    while (true) {
        int msg_len = recv(connect_sock, buffer, sizeof(buffer) - 1, 0);
        if (msg_len > 0) {
            for (int i = 0; i < msg_len; ++i) buffer[i] ^= XOR_KEY;
            buffer[msg_len] = '\0';
            string msg = buffer;

            lock_guard<mutex> lock(cout_mutex);
            cout << msg << endl;
            
            if (msg.find("CMD_KICK:") != string::npos) {
                cout << "You are kicked.\nEnter command (/create, /join): ";
            } else {
                cout << "Enter a message: ";
            }
            cout.flush();
        }
        else if (msg_len == 0) {
            lock_guard<mutex> lock(cout_mutex);
            cout << "\nDisconnected from server." << endl;
            exit(0);
        } else {
            lock_guard<mutex> lock(cout_mutex);
            cerr << "\nReceive failed!" << endl;
            exit(-1);
        }
    }
}

void ChatClient::handleConnection() {
    thread receiver(&ChatClient::receiveMessage, this);

    string input;
    while (true) {
        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Enter a message: ";
        }
        getline(cin, input);

        if (input == "/help") {
            showHelp();
            continue;
        }

        if (input == "/quit") {
            sendMessage("/quit");
            closesocket(connect_sock);
            WSACleanup();
            exit(0);
        }

        sendMessage(input);
    }
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
    showHelp();

    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Enter username: ";
    }
    getline(cin, username);

    while (true) {
        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Enter command (/create <group>, /join <group>): ";
        }
        string input;
        getline(cin, input);
        if (input == "/list") {
            sendMessage("/list");
            continue;
        }
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