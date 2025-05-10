Chat Application README
A TCP chat with timestamps, user status checks, and message history (Windows/MinGW)

Features:
- Group chat with create/join/leave functionality
- Message timestamps ([HH:MM] before each message)
- Individual user status checking (/userstatus)
- Persistent message history saved to chatlog.txt
- User management (mute/unmute/kick)
- XOR message encryption
- Automatic cleanup of inactive users

Compilation:
g++ Server.cpp -o server.exe -lws2_32 && g++ Client.cpp -o client.exe -lws2_32

Usage:
1. Start server: ./server.exe
   - Creates chatlog.txt automatically
2. Start clients: ./client.exe <server_ip> <port>

When client starts:
Enter username: <YourName>
Enter initial group: <group1>

Commands:
Basic:
/create <group>    - Create new group
/join <group>      - Join existing group
/leave             - Leave current group
/quit              - Exit client
/help              - Show help

Information:
/list_all_users    - Show all users and their groups
/userstatus <user> - Check if a user is online/offline
/history           - View last 20 messages from chatlog.txt

Owner Commands:
/groupuser <user>  - Add user to group
/mute <user>       - Mute user in group
/unmute <user>     - Unmute user in group
/kick <user>       - Kick user from group
/dismiss           - Dismiss entire group

All messages:
- Automatically timestamped by server
- Saved to chatlog.txt
- XOR-encrypted during transmission

Demo Tips:
1. Check user status: /userstatus Alice
2. View message history: /history
3. Messages show as "[14:30] User: Hello"
4. Server automatically saves all messages