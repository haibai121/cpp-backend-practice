#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<cstring>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<atomic>

#define BUFFER_SIZE 1024
#define PORT 9090

std::atomic<bool> client_running{true};

// ====== 接收消息的线程函数 ======
void receiveMessages(int sock_fd) {
    char buffer[BUFFER_SIZE];
    while (client_running) {
        int bytes = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "\n[收到] " << buffer << std::endl;
            std::cout << "请输入消息(quit退出): ";
        }else {
            std::cout << "\n服务器断开连接" << std::endl;
            break;
        }
    }
}

int main(){
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_port = PORT;
    serv_addr.sin_addr.s_addr = AF_INET;


    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        std::cerr << "初始化地址失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    std::cout << "初始化地址成功" << std::endl;

    //连接到服务器
    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "连接失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    std::cout << "连接到服务器" << std::endl;

    std::thread recv_thread(receiveMessages, sock_fd);

    std::cout << "请输入消息(quit退出): ";
    std::string message;
    while(client_running){
         std::getline(std::cin, message);
        if (message == "quit") {
            client_running = false;
            shutdown(sock_fd, SHUT_RDWR);
            break;
        }
        if (send(sock_fd, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "发送失败" << std::endl;
            break;
        }
    }

    close(sock_fd);
    if (recv_thread.joinable()) {
        recv_thread.join(); 
    }
    std::cout << "客户端已退出" << std::endl;
    return 0;

}