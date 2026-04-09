#include "../include/http_server.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        // 创建服务器实例
        Server server(8080, "../www"); // 端口 8080, 根目录 ./www
        // 启动服务器
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}