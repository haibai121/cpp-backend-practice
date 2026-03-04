#include<iostream>
#include<cstring>
#include<mutex>
#include<sys/socket.h>
#include<atomic>
#include<netinet/in.h>
#include<thread>
#include<vector>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9090
#define BUFFER_SIZE 1024
#define MAX_CLIENT 5


bool server_running = true;
std::vector<int> clients;
std::mutex clients_mutex;

void handleClient(int client_fd){
    char buffer[BUFFER_SIZE] = {0};
    while(server_running){
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            std::cout << "收到消息: " << buffer << std::endl;
            broadcast(buffer, client_fd); 
        }else if(bytes_read == 0){
            std::cout << "客户端断开 (fd=" << client_fd << ")" << std::endl;
            break;
        }else {
            std::cerr << "recv 错误" << std::endl;
            break;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for(auto& fd : clients){
            if(fd == client_fd){
                fd = -1;
                break;
            }
        }
        close(client_fd);
    }

}

void broadcast(const char* message, int sender_fd){
    std::lock_guard<std::mutex> lock(clients_mutex);

    if(clients.size() == 1 && clients[0] == sender_fd){
        send(sender_fd, message, strlen(message), 0);
    }

    for(int fd : clients){
        if(fd != sender_fd && fd != -1){
            if(send(fd, message, strlen(message), 0) < 0){
                std::cerr << "向 fd=" << fd << " 发送失败" << std::endl;
            }
        }
    }
}

void listenForShutdown() {
    std::string input;
    std::getline(std::cin, input);
    if (input == "quit") {
        server_running = false;
        std::cout << "正在关闭服务器..." << std::endl;
    }
}

int main(){
    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_fd < 0){
        std::cerr << "s-1" << std::endl;
        return -1;
    }
    std::cout << "s0" << std::endl;

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(serv_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        std::cerr << "b-1" << std::endl;
        return -1;
    }
    std::cout << "b0" << std::endl;

    if(listen(serv_fd, MAX_CLIENT) < 0){
        std::cerr << "l-1" << std::endl;
        return -1;
    }
    std::cout << "l0" << std::endl;

    while(server_running){
        //因为accpet需要接收的是sockaddr类型的指针
        //但是client_addr被定义的时候需要sockaddr_in这个类型
        //所以在accept里需要强制转换类型为sockaddr*类型
        //client_len之所以不转换是因为定义的时候就已经是socklen_t类型，所以只需要取其地址即可
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(serv_fd, (struct sockaddr*)&client_addr, &client_len);
        if(client_fd < 0){
            if(!server_running){
                break;
            }

            if (server_running) std::cerr << "a-1" << std::endl;
            continue;
        }

        std::thread(handleClient, client_fd).detach();

    }

    close(serv_fd);
    return 0;
    
}