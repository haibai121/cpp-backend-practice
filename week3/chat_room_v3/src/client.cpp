#include <iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<thread>
#include<mutex>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<atomic>
#include<cstring>

#define PORT 8080
#define BUFFER_SIZE 1024
std::atomic<bool> client_running {true};

void ReciveMessage(int sock_fd){
    char buffer[BUFFER_SIZE] = {0};
    while(client_running){
        int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE-1, 0);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            std::cout << "\n" << buffer << std::flush; // 👈 加 flush
            std::cout << "请输入消息(quit退出): " << std::flush;
        }else{
            std::cout << "\n服务器断开连接" << std::endl;
            client_running = false; // 👈 关键！通知主线程退出
            break;
        }
    }
}

int main(){
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_family = AF_INET;

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        std::cerr << "初始化地址失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    std::cout << "初始化地址成功" << std::endl;

    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "连接失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    std::cout << "连接到服务器" << std::endl;

    std::thread recv_thread(ReciveMessage, sock_fd);

    std::cout << "请输入你的名字：" << std::endl;
    std::string name;
    std::getline(std::cin, name);
    if(send(sock_fd, name.c_str(), name.length(), 0) < 0){
        std::cout << "发送失败" << std::endl;
    }
    std::cout << "请输入消息(quit退出): ";
    std::string message;
    while(client_running){
        std::getline(std::cin, message);
        if(message == "quit"){
            client_running = false;
            shutdown(sock_fd, SHUT_RDWR);
            break;
        }
        if(send(sock_fd, message.c_str(), message.length(), 0) < 0){
            std::cerr << "发送失败" << std::endl;
            break;
        }
    }

    close(sock_fd);
    if(recv_thread.joinable()){
        recv_thread.join();
    }
    std::cout << "客户端已退出" << std::endl;
    return 0;
}