#include "/home/cppdev/projects/cpp-backend-practice/week4/file_transfer_server/include/file.hpp"
#include <iostream>
#include <chrono>
#include <csignal>
#include <cstdlib> 

bool g_shutdown = false;

void signal_handler(int sig) {
    g_shutdown = true;
    std::cout << "\nReceived signal " << sig << ". Shutting down...\n";
}

int main(){
    std::signal(SIGINT,  signal_handler);   
    std::signal(SIGTERM, signal_handler);   

    try{
        FileServer fileserver;
        fileserver.run(); // <--- 调用 fileserver 对象的 run 方法
    }catch(const std::exception& e){ // 顺便，捕获 const 引用更佳
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}