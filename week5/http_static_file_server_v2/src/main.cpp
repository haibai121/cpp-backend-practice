#include "../include/Http_server.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        // 创建服务器实例
        Server server(8081, "../www"); // 端口 8081, 根目录 ../www
        // 启动服务器
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}