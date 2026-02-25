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
#include <vector>
#include <mutex>

#define BUFFER_SIZE 1024
#define PORT 9090
#define MAX_CLIENT 5

// ====== 【新增】全局变量：管理所有客户端 ======
// 原因：你需要一个地方存所有连接的 fd，才能广播
std::vector<int> clients;
std::mutex clients_mutex;               //新增线程安全锁，因为对于便利客户端容器的这个操作属于对临界资源进行操作，需要对数据进行上锁
bool server_running = true;             //控制服务器是否运行（用于优雅退出而不是Ctrl+C）

// ====== 【新增】广播函数 ======
// 功能：把消息发给除 sender_fd 外的所有人
void broadcast(const char* message, int sender_fd){
    std::lock_guard<std::mutex> lock(clients_mutex);  //自动加锁/解锁
    for(int fd : clients){
        if(fd != sender_fd && fd != -1){ //不发给自己，跳过已关闭的客户端
            if(send(fd, message, strlen(message), 0) < 0){
                std::cerr << "向 fd=" << fd << " 发送失败" << std::endl;
            }
        }
    }
}

// ====== 修改 HandleMessage → 改名为 handleClient 并支持广播 ======
// 原问题：只 echo 回发，不能广播
//改名为Handle_Message是因为怕函数名与后面代码的函数重载，所以改名，原本叫HandleMessage
int Handle_Message(int client_fd){
//     while(true){
//         char buffer[BUFFER_SIZE] = {0};
//         int bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);
//         if(bytes_read > 0){
//             if(send(client_fd, buffer, BUFFER_SIZE, 0) < 0){
//                 std::cout << "信息发送失败" << std::endl;
//                 close(client_fd);
//                 return -1;
//             }
//             std::cout << "信息广播成功" << std::endl;
//             continue;
//         }else if(bytes_read == 0){
//             std::cout << "客户端关闭" << std::endl;
//             close(client_fd);
//             break;
//         }else {
//             std::cout << "出错了" << std::endl;
//             close(client_fd);
//             return -2;
//         }
//     }
//     return 1;
}

void handleClient(int client_fd) {
    char buffer[BUFFER_SIZE];
    while(server_running){
        //revc处理的更严谨
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';//这里有疑惑，为什么bytes_read是数组最后一个
            std::cout << "收到消息: " << buffer << std::endl;
            broadcast(buffer, client_fd); // 改为广播，不是回发
        }else if(bytes_read == 0){
            std::cout << "客户端断开 (fd=" << client_fd << ")" << std::endl;
            break;
        }else {
            std::cerr << "recv 错误" << std::endl;
            break;
        }
    }
    //加锁，这里用{}的原因是：{}相当于是一个作用域，等执行完{}里面的语句就会自动析构，然后就会解锁
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto& fd : clients) {
            if (fd == client_fd) {
                fd = -1; // 标记为无效（简单做法，也可用 erase）
                break;
            }
        }
    }
    close(client_fd);
}

// ====== 【新增】监听键盘输入，用于关闭服务器 ======
void listenForShutdown() {
    std::string input;
    std::getline(std::cin, input);
    if (input == "quit") {
        server_running = false;
        std::cout << "正在关闭服务器..." << std::endl;
    }
}

// std::string getCurrentTime(){
//     //获取当前时间
//     auto now = std::chrono::system_clock::now();

//     //转换为time_t(c风格)
//     auto time = std::chrono::system_clock::to_time_t(now);

//     //转换为字符串
//     std::stringstream ss;
//     ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
//     return ss.str();
// }

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        std::cerr << "socket创建失败" << std::endl;
        return -1;
    }
    std::cout << "Socket创建成功" << std::endl;

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //这里强制类型转换有问题，为什么转换为sockaddr类型的指针而不是什么定义的sockaddr_in类型的指针?
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "绑定失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "绑定端口：" << PORT << std::endl;

    if(listen(server_fd, MAX_CLIENT) < 0){
        std::cerr << "监听失败" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "成功监听端口:" << PORT << std::endl;

    // 👇 【新增】启动关闭监听线程
    std::thread shutdown_thread(listenForShutdown);

    while (server_running){
        // ✅ 正确的 accept 用法：
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);


        if (client_fd < 0) {
            if (server_running) std::cerr << "accept 失败" << std::endl;
            continue;
        }

        // 👇 检查是否超过最大连接数（线程安全）
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if(clients.size() >= MAX_CLIENT ){
                std::cout << "达到最大连接数，拒绝新连接" << std::endl;
                close(client_fd);
                continue;
            }
            clients.push_back(client_fd);//这句？
        }

        std::cout << "新客户端连接 (fd=" << client_fd << ")" << std::endl;
        // ❌ 你原来的 t1.join() 会让服务器卡住！只能服务一个客户
        // ✅ 改为 detach() 或管理线程池
        std::thread(handleClient, client_fd).detach();//这里用的线程池？
    }

    // 👇 优雅关闭：关闭所有客户端 + 服务器 socket
    {
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (int fd : clients) {
            if (fd != -1) close(fd);
        }
        clients.clear();
    }
    close(server_fd);
    shutdown_thread.join(); // 等待关闭线程结束

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}