#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#define BUFFER_SIZE 1024
#define PORT 8080

std::string getCurrentTime(){
    //获取当前时间
    auto now = std::chrono::system_clock::now();

    //转换为time_t(C 风格时间)
    auto time = std::chrono::system_clock::to_time_t(now);

    //格式化为字符串
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "Socket创建成功" << std::endl;


    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "绑定失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "绑定端口：" << PORT << std::endl;


    if(listen(server_fd, 3) < 0){
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "成功监听端口：" << PORT << std::endl;
    

    
    while(true){
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd < 0){
            std::cerr << "请求失败" << std::endl;
            continue;
        }
        std::cout << "客户端连接成功(fd=" << client_fd << ")" << std::endl;

        std::string time = getCurrentTime();
        send(client_fd, time.c_str(), time.length(), 0);
        std::cout << "当前时间为:" << time << "已发送" << std::endl;


        char buffer[BUFFER_SIZE] = {0};
        
        int byte_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if(byte_read > 0){
            std::cout << "接收到:" << buffer << std::endl;

            send(client_fd, buffer, BUFFER_SIZE , 0);
            std::cout << "回显发送" << std::endl;
        }else if(byte_read == 0){
            std::cout << "客户端关闭连接" << std::endl;
        }else {
            std::cout << "连接失败" << std::endl;
        }

        close(client_fd);
        std::cout << "客户端连接已关闭" << std::endl;
    }


    close(server_fd);
    return 0;
}