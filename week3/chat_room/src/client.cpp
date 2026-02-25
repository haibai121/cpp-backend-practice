#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<cstring>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>

#define BUFFER_SIZE 1024
#define PORT 9090

// ====== 【新增】接收消息的线程函数 ======
// 原问题：你只发一次就退出，不能持续聊天
void receiveMessages(int sock_fd) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "\n[收到] " << buffer << std::endl;
            std::cout << "请输入消息(quit退出): "; // 提示用户继续输入
        } else {
            std::cout << "\n服务器断开连接" << std::endl;
            break;
        }
    }
}

int main(){
    //创建socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    //配置服务器地址
    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    //将IP地址由字符串转换为二进制
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

    // ❌ 你原来的代码只调用了一次 send_message 就退出了！
    // ✅ 现在：启动接收线程 + 主线程发消息

    std::thread recv_thread(receiveMessages, sock_fd); // 启动接收线程

    std::cout << "请输入消息(quit退出): ";
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "quit") {
            break;
        }
        // 👇 原来 send 用了 BUFFER_SIZE，但实际只发 message.length()
        if (send(sock_fd, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "发送失败" << std::endl;
            break;
        }
    }

    close(sock_fd);
    if (recv_thread.joinable()) {
        recv_thread.join(); // 等待接收线程结束
    }
    std::cout << "客户端已退出" << std::endl;
    return 0;

}