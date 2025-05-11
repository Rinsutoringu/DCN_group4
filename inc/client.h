#pragma once

#include <winsock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#define XOR_KEY 0xAB

class ChatClient {
public:
    /**
     * 构造函数
     * @param server_name 服务器名称
     * @param port 服务器端口
     */
    ChatClient(const char* server_name, unsigned short port);
    
    /**
     * 入口函数，启动客户端
     */
    void start();

private:
    // 连接的socket
    SOCKET connect_sock;
    // socket地址结构体
    struct sockaddr_in server_addr;
    // Winsock数据结构
    WSADATA wsaData;
    // 输出流锁
    std::mutex cout_mutex;
    std::mutex input_mutex;
    // 用户名
    std::string username;
    // 通知其他进程：获取到了消息
    std::condition_variable input_cv;
    // 队列，存储输入消息
    std::queue<std::string> input_queue;
    // 标志位，表示是否退出
    bool exit_flag = false;

    /**
     * XOR加密
     * @param data 要加密的数据
     * @return 加密后的数据
     */
    std::string xorCipher(const std::string& data);

    /**
     * 发送消息
     * @param message 要发送的消息
     */
    void sendMessage(const std::string& message);

    /**
     * 接收消息
     */
    void receiveMessage();

    /**
     * 处理连接
     */
    void handleConnection();

    /**
     * 显示帮助
     */
    void showHelp();

    /**
     * 检查连接状态
     * @return 掉了就是false
     */
    bool connStatus();

    void getMessage();
};