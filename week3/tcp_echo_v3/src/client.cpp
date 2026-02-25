#include<iostream>
#include<sys/socket.h>
#include<cstring>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main(){
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "socket创建成功" << std::endl;

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0){
        std::cerr << "初始化地址失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    
    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "连接失败" << std::endl;
        close(sock_fd);
        return -1;
    }
    std::cout << "连接到服务器" << std::endl;

    while(true){
        char buffer[BUFFER_SIZE] = {0};
        int time_read = recv(sock_fd, buffer, BUFFER_SIZE, 0);
        if(time_read > 0){
            std::cout << "来自服务器发送的时间" << buffer << std::endl;
        }
        std::cout << "发送消息（'quit'类型为退出）：";
        std::string message;
        std::getline(std::cin, message);

        if(message == "quit"){
            std::cout << "退出中..." << std::endl;
            break;
        }

        if(send(sock_fd, message.c_str(), message.length(), 0) < 0){
            std::cerr << "发送失败" << std::endl;
            break;
        }
        std::cout << "消息发送成功" << std::endl;

        int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE, 0);
        if(bytes_read > 0) {
            std::cout << "来自服务器的回显" << buffer << std::endl;
        }else if(bytes_read == 0){
            std::cout << "服务器已关闭" << std::endl;
            break;
        }else {
            std::cerr << "接收失败" << std::endl;
            break;
        }


    //5.关闭连接
    close(sock_fd);
    std::cout << "连接已关闭" << std::endl;
    return 0;
    }

    

}