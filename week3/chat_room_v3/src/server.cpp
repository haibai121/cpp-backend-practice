#include<iostream>
#include<thread>
#include<mutex>
#include<atomic>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<cstring>
#include<vector>
#include<netinet/in.h>
#include <algorithm>
#include<map>
//时间
#include <chrono>
#include <iomanip>
#include <sstream>

#define MAX_CLIENT 5
#define BUFFER_SIZE 1024
#define PORT 8080

bool server_running = true;
// std::vector<int> clients;
std::mutex client_mutex;
std::map<int, std::string> clients;

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm;
    localtime_r(&time_t, &tm); // Linux 安全版本（线程安全）

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S"); // 格式：13:45:22
    return oss.str();
}

void BroadcastMessage(const char* message, int sender_fd) {
    std::lock_guard<std::mutex> lock(client_mutex);
    
    auto it = clients.find(sender_fd);
    if (it == clients.end()) {
        return; 
    }
    std::string sender_name = it->second;
    std::string time_str = getCurrentTime(); // 获取时间

    std::string full_message = "[" + time_str + "] [" + sender_name + "] " + std::string(message) + "\n";

    if (clients.size() == 1) {
        send(sender_fd, full_message.c_str(), full_message.length(), 0);
        return;
    }

    for (const auto& [fd, name] : clients) {
        if (fd != sender_fd) {
            if (send(fd, full_message.c_str(), full_message.length(), 0) < 0) {
                std::cerr << "向 [" << name << "] 发送消息失败" << std::endl;
            }
        }
    }
}

void HandleClients(int client_fd){
    char buffer[BUFFER_SIZE] = {0};
    while(server_running){
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            std::cout << "收到" << clients[client_fd] << "的消息: " << buffer << std::endl;
            BroadcastMessage(buffer, client_fd);
        }else if(bytes_read == 0){
            std::cout << "客户端断开 (fd=" << client_fd << ", name=" << clients[client_fd] << ")" << std::endl;
            break;
        }else {
            std::cerr << "recv 错误" << std::endl;
            break;
        }
    }
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        clients.erase(client_fd);
    }
    close(client_fd);
    
}

void listenForShutdown(){
    std::string status;
    std::getline(std::cin, status);
    if(status == "quit"){
        server_running = false;
        std::cout << "正在关闭服务器..." << std::endl;
    }

}

int main(){
    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "Socket创建成功" << std::endl;
    struct sockaddr_in address;
    // std::memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if(bind(serv_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "绑定失败" << std::endl;
        close(serv_fd);
        return -1;
    }
    std::cout << "绑定端口：" << PORT << std::endl;

    // struct timeval tv = {60, 0};
    // if (setsockopt(serv_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    //     std::cerr << "设置超时失败" << std::endl;
    // }

    if(listen(serv_fd, MAX_CLIENT) < 0){
        std::cerr << "监听失败" << std::endl;
        close(serv_fd);
        return -1;
    }
    std::cout << "成功监听端口:" << PORT << std::endl;

    std::thread shutdown_thread(listenForShutdown);

    while(server_running){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(serv_fd, (struct sockaddr*)&client_addr, &client_len);

        if(client_fd < 0){
            if(!server_running){
                break;
            }
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
                continue; 
            }
            if (server_running) std::cerr << "accept 失败" << std::endl;
            continue;
        }
        char buffer[25] = {0};
        int name_bytes = recv(client_fd, buffer, 25-1, 0);
        buffer[name_bytes] = '\0';
        std::string name = buffer;
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            if(clients.size() >= MAX_CLIENT){
                std::cout << "达到最大连接数，拒绝新连接" << std::endl;
                close(client_fd);
                continue;
            }
            clients.insert({client_fd, name});
            
        }

        std::cout << "新客户端连接成功(fd=" << client_fd << ", name=" << name << ")" <<std::endl;
        std::thread(HandleClients, client_fd).detach();
    }
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        for(auto& [fd, name] : clients){
            close(fd);
        }
        clients.clear();
    }
    close(serv_fd);
    shutdown_thread.join();

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}