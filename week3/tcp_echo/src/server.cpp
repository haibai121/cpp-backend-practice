#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8080

int main(){
    // 1.创建socket
    // AF_INET: IPV4
    // SOCK_STREAM: TCP协议
    // 0: 自动选择协议

    //这句目前不知道什么意思，应该是启动的协议还是啥
    //如果小于0的话说明没启动成功
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(server_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "Socket创建成功" << std::endl;


    //2.配置服务器地址
    //这一段不知道什么意思
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));  // 清零
    address.sin_family = AF_INET;               // IPV4
    address.sin_addr.s_addr = INADDR_ANY;       // 监听所有网卡
    address.sin_port = htons(PORT);             // 端口号（网络字节序）


    //3.绑定地址和端口
    //这一段不知道什么意思
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "绑定失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "绑定端口：" << PORT << std::endl;


    //4.开始监听
    //backlog：等待连接队列的最大长度
    if(listen(server_fd, 3) < 0){                       //这段判断的语句不懂
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "成功监听端口：" << PORT << std::endl;


    //5.循环接受客户端连接
    while(true){
        //accept 会阻塞，知道所有客户端连接
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd < 0){
            std::cerr << "请求失败" << std::endl;
            continue;
        }
        std::cout << "客户端连接成功(fd=" << client_fd << ")" << std::endl;


        //6.接收和回显数据
        char buffer[BUFFER_SIZE] = {0};

        //revc会阻塞，知道收到数据
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if(bytes_read > 0){
            std::cout << "接收到：" << buffer << std::endl;

            //原样会显给客户端
            send(client_fd, buffer, bytes_read, 0);
            std::cout << "Echo 发送" << std::endl;
        }else if(bytes_read == 0) {
            std::cout << "客户端关闭连接" << std::endl;
        }else {
            std::cerr << "接收失败" << std::endl;
        }


        //7.关闭客户端连接
        close(client_fd);
        std::cout << "客户端连接已关闭" << std::endl;
    }


    //8.关闭服务器scoket
    close(server_fd);
    return 0;
}