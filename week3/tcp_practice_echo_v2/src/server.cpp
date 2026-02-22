#include<iostream>
#include<sys/socket.h>
#include<cstring>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main(){
    //1.创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "socket创建成功" << std::endl;


    //2.配置服务器地址
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //3.绑定地址和端口
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "绑定失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "绑定端口：" << PORT << std::endl;

    //4.开始监听
    //backlog:等待连接队列的最大长度
    if(listen(server_fd, 3) < 0){
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "成功监听端口：" << PORT << std::endl;


    //5.循环等待客户端连接
    //accpet会阻塞，知道所有客户端连接
    int client_fd = accept(server_fd, nullptr, nullptr);
    while(true){
        
        if(client_fd < 0){
            std::cerr << "请求失败" << std::endl;
            continue;
        }
        std::cout << "客户端连接成功，编号为：" << client_fd << std::endl;

        //6.接收和回显数据
        char buffer[BUFFER_SIZE] = {0};

        //recv会阻塞，直到收到数据
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if(bytes_read > 0){
            std::cout << "接收到：" << buffer << std::endl;

            //原样回显给客户端
            send(client_fd, buffer, bytes_read, 0);
            std::cout << "回显发送" << std::endl;
        }else if(bytes_read == 0){
            std::cout << "客户端关闭连接" << std::endl;
            break;
        }else {
            std::cout << "接收失败" << std::endl;
            break;
        }
    }
    //7.关闭客户端连接
    close(client_fd);

    //8.关闭服务器socket
    close(server_fd);
    return 0;
    
}