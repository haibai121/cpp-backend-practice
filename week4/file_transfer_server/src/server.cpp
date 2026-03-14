#include "/home/cppdev/projects/cpp-backend-practice/week4/file_transfer_server/include/file.hpp"
#include <iostream>
#include <fstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <unordered_map>
#include <fcntl.h>
#include <vector>
#include <cstring> // For memcpy
#include <sys/stat.h>

extern bool g_shutdown;

namespace {
    void set_nonblock(int fd) {
        int flag = ::fcntl(fd, F_GETFL, 0);
        if (flag == -1 || ::fcntl(fd, F_SETFL, flag | O_NONBLOCK)) { // Fixed: Use '|' instead of '||'
            throw std::runtime_error("Failed to set nonblock");
        }
    }

    class FileDescriptor {
        int fd_;
    public:
        FileDescriptor(int fd = -1) : fd_(fd) {}
        ~FileDescriptor() { if (fd_ >= 0) ::close(fd_); }

        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
        FileDescriptor& operator=(FileDescriptor&& other) noexcept {
            if (this != &other) {
                if (fd_ >= 0) ::close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const { return fd_; }
        explicit operator bool() const { return fd_ >= 0; }
    };

    // 定义协议头结构
    struct FileTransferHeader {
        uint64_t file_size;
        uint32_t filename_len;
    };

    // 定义客户端连接的状态
    enum class TransferState {
        READING_HEADER,
        READING_FILENAME,
        READING_FILE_DATA
    };

    // 存储单个客户端的传输状态
    struct ClientSession {
        TransferState state = TransferState::READING_HEADER;
        FileTransferHeader header{};
        std::string filename;
        std::ofstream file_stream;
        size_t bytes_received_for_current_part = 0; // 当前状态已接收的字节数
        size_t total_bytes_received = 0; // 总共接收到的文件数据字节数
    };
}

struct FileServer::Impl {
    FileDescriptor server_fd;
    FileDescriptor epfd;
    std::unordered_map<int, ClientSession> clients; // Changed: Store session data instead of just fd
    static constexpr int PORT = 8083;
    static constexpr int MAX_EVENTS = 1024;
};

FileServer::FileServer() : pimpl_(std::make_unique<Impl>()) {

    // --- 新增：创建 uploads 目录 ---
    static const std::string UPLOADS_DIR = "../uploads";
    
    // 检查目录是否存在
    struct stat st;
    if (stat(UPLOADS_DIR.c_str(), &st) != 0) {
        // 目录不存在，尝试创建
        if (mkdir(UPLOADS_DIR.c_str(), 0755) != 0) {
            throw std::runtime_error("Failed to create upload directory: " + UPLOADS_DIR);
        }
        std::cout << "Created upload directory: " << UPLOADS_DIR << std::endl;
    } else {
        // 目录已存在，检查是否为目录
        if (!S_ISDIR(st.st_mode)) {
            throw std::runtime_error("Path exists but is not a directory: " + UPLOADS_DIR);
        }
    }
    // --- 新增结束 ---

    pimpl_->server_fd = FileDescriptor(::socket(AF_INET, SOCK_STREAM, 0));
    if (!pimpl_->server_fd) {
        throw std::runtime_error("socket() failed");
    }

    int opt = 1;
    ::setsockopt(pimpl_->server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(pimpl_->PORT);

    if (::bind(pimpl_->server_fd.get(), (struct sockaddr*)&addr, sizeof(addr)) == -1 ||
        ::listen(pimpl_->server_fd.get(), 5) == -1) {
        throw std::runtime_error("bind() or listen() failed");
    }

    set_nonblock(pimpl_->server_fd.get());

    pimpl_->epfd = FileDescriptor(::epoll_create1(EPOLL_CLOEXEC));
    if (!pimpl_->epfd) {
        throw std::runtime_error("epoll_create1() failed");
    }

    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = pimpl_->server_fd.get();
    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, pimpl_->server_fd.get(), &ev) == -1) {
        throw std::runtime_error("epoll_ctl() failed");
    }

    std::cout << "File transfer server listening on port " << Impl::PORT << "\n" << std::endl;
}

FileServer::~FileServer() = default;

void FileServer::remove_client(int fd) {
    // 从 epoll 中移除
    ::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_DEL, fd, nullptr);
    // 从 map 中移除，其析构函数会自动关闭 fd
    pimpl_->clients.erase(fd);
    std::cout << "Client (fd=" << fd << ") disconnected.\n" << std::endl;
}

void FileServer::handle_client_data(int fd) {
    auto it = pimpl_->clients.find(fd);
    if (it == pimpl_->clients.end()) return; // Should not happen

    ClientSession& session = it->second;

    char buffer[4096];
    int bytes_read = ::recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // 对端关闭连接
            remove_client(fd);
        }
        return; // Error or would block
    }

    int processed = 0;
    while (processed < bytes_read) {
        if (session.state == TransferState::READING_HEADER) {
            size_t remaining = sizeof(session.header) - session.bytes_received_for_current_part;
            int to_copy = std::min(static_cast<size_t>(bytes_read - processed), remaining);

            char* header_ptr = reinterpret_cast<char*>(&session.header);
            memcpy(header_ptr + session.bytes_received_for_current_part, buffer + processed, to_copy);

            session.bytes_received_for_current_part += to_copy;
            processed += to_copy;

            if (session.bytes_received_for_current_part == sizeof(session.header)) {
                // 协议头接收完毕
                // 替换掉对 ntohll 的调用
                session.header.file_size = ((uint64_t)ntohl(session.header.file_size & 0xFFFFFFFF) << 32) | ntohl((session.header.file_size >> 32) & 0xFFFFFFFF);// 网络字节序转为主机字节序
                session.header.filename_len = ntohl(session.header.filename_len);

                session.bytes_received_for_current_part = 0; // 重置计数器
                session.state = TransferState::READING_FILENAME;
                
                std::cout << "Header received for client " << fd << ". File size: " << session.header.file_size 
                          << ", Filename length: " << session.header.filename_len << std::endl;
            }
        }
        else if (session.state == TransferState::READING_FILENAME) {
            size_t remaining = session.header.filename_len - session.bytes_received_for_current_part;
            int to_copy = std::min(static_cast<size_t>(bytes_read - processed), remaining);

            session.filename.append(buffer + processed, to_copy);

            session.bytes_received_for_current_part += to_copy;
            processed += to_copy;

            if (session.bytes_received_for_current_part == session.header.filename_len) {
                // 文件名接收完毕

                // --- 修改开始：设置固定存储目录 ---
                static const std::string STORAGE_DIR = "../uploads";

                // 为了安全，只保留文件名部分，去掉路径
                auto last_slash_pos = session.filename.find_last_of("/\\");
                std::string final_filename;
                if (last_slash_pos != std::string::npos) {
                    final_filename = session.filename.substr(last_slash_pos + 1);
                } else {
                    final_filename = session.filename;
                }

                // 组合新的完整路径
                session.filename = STORAGE_DIR + "/" + final_filename;
                // --- 修改结束 ---

                session.bytes_received_for_current_part = 0; // 重置计数器
                session.state = TransferState::READING_FILE_DATA;

                // 打开文件准备写入
                session.file_stream.open(session.filename, std::ios::binary);
                if (!session.file_stream) {
                    std::cerr << "Failed to open file '" << session.filename << "' for writing." << std::endl;
                    remove_client(fd); // Error handling
                    return;
                }
                
                std::cout << "Filename received for client " << fd << ": " << session.filename << std::endl;
            }
        }
        else if (session.state == TransferState::READING_FILE_DATA) {
            size_t remaining_in_buffer = bytes_read - processed;
            size_t remaining_in_file = session.header.file_size - session.total_bytes_received;
            int to_write = std::min(remaining_in_buffer, static_cast<size_t>(remaining_in_file));

            session.file_stream.write(buffer + processed, to_write);
            session.total_bytes_received += to_write;
            processed += to_write;

            if (session.total_bytes_received == session.header.file_size) {
                // 文件接收完毕
                session.file_stream.close();
                std::cout << "File transfer completed for client " << fd << ", saved as '" << session.filename << "'" << std::endl;
                
                // 重置状态，准备接收下一个文件（或断开连接）
                session.state = TransferState::READING_HEADER;
                session.bytes_received_for_current_part = 0;
                session.total_bytes_received = 0;
                session.header = {};
                session.filename.clear();
            }
        }
    }
}


void FileServer::run() {
    std::vector<struct epoll_event> events(Impl::MAX_EVENTS);
    while (!g_shutdown) {
        int nready = ::epoll_wait(pimpl_->epfd.get(), events.data(), static_cast<int>(events.size()), 500);
        if (nready < 0) continue;

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            if (fd == pimpl_->server_fd.get()) {
                // New connection
                while (true) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = ::accept4(pimpl_->server_fd.get(), (struct sockaddr*)&client_addr, &client_len, SOCK_NONBLOCK);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        else {
                            std::cerr << "Accept failed. \n" << std::endl;
                            continue;
                        }
                    }

                    std::cout << "New connection (fd = " << client_fd << ")\n" << std::endl;

                    struct epoll_event ev{};
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN;
                    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        ::close(client_fd);
                        std::cerr << "Failed to add client to epoll_ctl()." << std::endl;
                        continue;
                    }

                    // Initialize a new client session
                    pimpl_->clients.emplace(client_fd, ClientSession{});
                }
            } else {
                // Existing client has data
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                    remove_client(fd);
                    continue;
                }

                handle_client_data(fd);
            }
        }
    }
}
