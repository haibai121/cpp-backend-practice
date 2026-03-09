// src/main.cpp
#include "echo.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib> // for exit()

// 全局标志：是否收到退出信号
bool g_shutdown = false;

// 简化的信号处理函数（教学用途，非生产级）
void signal_handler(int sig) {
    g_shutdown = true;
    // 注意：这里仍用 std::cout，但在简单单线程程序中通常能工作
    std::cout << "\nReceived signal " << sig << ". Shutting down...\n";
}

int main() {
    // 注册信号处理器
    std::signal(SIGINT,  signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);   // kill 命令

    try {
        EchoServer server;
        
        // 启动服务器（假设 run() 是永久阻塞的）
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}