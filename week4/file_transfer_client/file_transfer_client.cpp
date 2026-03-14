#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring> // For memset
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// 定义与服务器相同的协议头结构
struct FileTransferHeader {
    uint64_t file_size;
    uint32_t filename_len;
};

// 辅助函数：将主机字节序转换为网络字节序
inline uint64_t htonll(uint64_t host) {
    // 检查主机是否为小端序
    union { uint32_t i; char c[4]; } u = {1};
    if (u.c[0]) { // 小端序
        return ((uint64_t)htonl(host & 0xFFFFFFFF) << 32) | htonl((host >> 32) & 0xFFFFFFFF);
    } else { // 大端序，无需转换
        return host;
    }
}

bool send_file_to_server(const std::string& server_ip, int server_port, const std::string& local_file_path) {
    // 1. 创建 socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    // 2. 设置服务器地址
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported: " << server_ip << std::endl;
        close(sockfd);
        return false;
    }

    // 3. 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed: " << strerror(errno) << std::endl;
        close(sockfd);
        return false;
    }
    std::cout << "Connected to server at " << server_ip << ":" << server_port << std::endl;

    // 4. 打开本地文件
    std::ifstream file(local_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << local_file_path << std::endl;
        close(sockfd);
        return false;
    }

    // 5. 获取文件大小
    file.seekg(0, std::ios::end);
    uint64_t file_size = file.tellg();
    file.seekg(0, std::ios::beg); // 重新回到文件开头

    // 6. 构建协议头
    FileTransferHeader header;
    header.file_size = htonll(file_size); // 发送前转换为网络字节序
    header.filename_len = htonl(local_file_path.length()); // 发送前转换为网络字节序

    // 7. 发送协议头
    if (send(sockfd, &header, sizeof(header), 0) != sizeof(header)) {
        std::cerr << "Error sending header: " << strerror(errno) << std::endl;
        file.close();
        close(sockfd);
        return false;
    }
    std::cout << "Header sent. File size: " << file_size << ", Filename: " << local_file_path << std::endl;

    // 8. 发送文件名
    if (send(sockfd, local_file_path.c_str(), local_file_path.length(), 0) != (ssize_t)local_file_path.length()) {
        std::cerr << "Error sending filename: " << strerror(errno) << std::endl;
        file.close();
        close(sockfd);
        return false;
    }
    std::cout << "Filename sent." << std::endl;

    // 9. 分块发送文件内容
    char buffer[4096];
    size_t total_sent = 0;
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        size_t bytes_to_send = file.gcount(); // gcount() 返回上次 read 成功读取的字节数
        ssize_t sent = send(sockfd, buffer, bytes_to_send, 0);
        if (sent == -1) {
            std::cerr << "Error sending file data: " << strerror(errno) << std::endl;
            file.close();
            close(sockfd);
            return false;
        }
        total_sent += sent;
        // std::cout << "Sent chunk of " << sent << " bytes. Total: " << total_sent << "/" << file_size << std::endl;
    }

    std::cout << "File sent successfully. Total bytes: " << total_sent << std::endl;

    // 10. 关闭连接
    file.close();
    close(sockfd);
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port> <local_file_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 8083 ./path/to/myfile.txt" << std::endl;
        return 1;
    }

    std::string server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    std::string local_file_path = argv[3];

    if (send_file_to_server(server_ip, server_port, local_file_path)) {
        std::cout << "File transfer completed." << std::endl;
    } else {
        std::cout << "File transfer failed." << std::endl;
        return 1;
    }

    return 0;
}