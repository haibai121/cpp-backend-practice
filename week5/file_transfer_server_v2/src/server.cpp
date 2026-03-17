#include "week5/file_transfer_server_v2/include/File.hpp"
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>

extern bool g_shutdown;

namespace{
    void ser_nonblock(int fd){
        int flag = ::fcntl(fd, F_GETFL, 0);
        if(flag == -1 || ::fcntl(fd, F_SETFL, flag | O_NONBLOCK)){
            throw std::runtime_error("Failed to set nonblock");
        }
    }

    class FileDescriptor{
        int fd_;
    public:
        FileDescriptor(int fd = -1) : fd_(fd){};
        ~FileDescriptor();

        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_){ other.fd_ = -1; }
        FileDescriptor& operator=(FileDescriptor&& other) noexcept {
            if(this != &other){
                if(fd_ >= 0) close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const { return fd_; }
        explicit operator bool() const { return fd_ >= 0; }
    };

    struct FileTransferHeader{
    uint64_t file_size;
    uint32_t filename_len;
    };

    enum class TransferState {
        READING_HEADER,
        READING_FILENAME,
        READING_FILE_DATA
    };

    struct ClientSession{
        TransferState state = TransferState::READING_HEADER;
        FileTransferHeader header{};
        std::string filename;
        std::ofstream file_stream;
        size_t bytes_received_for_current_part = 0;
        size_t total_bytes_received = 0;
    };
}

struct FileServer::Impl{
    FileDescriptor server_fd;
    FileDescriptor epfd;
    std::unordered_map<int, ClientSession> clients;
    static constexpr int PORT = 8084;
    static constexpr int MAX_EVENTS = 1024;
};

FileServer::FileServer() : pimpl_(std::make_unique<Impl>()){
    FileDescriptor server_fd = FileDescriptor(::socket(AF_INET, SOCK_STREAM, 0));
    if (!pimpl_->server_fd) {
        throw std::runtime_error("socket() failed");
    }

    struct sockaddr_in addr{};
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(pimpl_->PORT);

    int opt = 1;
    ::setsockopt(pimpl_->server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(pimpl_->server_fd.get(), (struct sockaddr*)&addr, sizeof(addr)) == -1 ||
        ::listen(pimpl_->server_fd.get(), 5) == -1) {
        throw std::runtime_error("bind() or listen() failed");
    }

    ser_nonblock(pimpl_->server_fd.get());

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

void FileServer::handle_client_data(int fd){
    auto it = pimpl_->clients.find(fd);
    if(it == pimpl_->clients.end()) return;

    ClientSession& session = it->second;

    char buffer[4096] = {};
    int bytes_read = ::recv(fd, buffer, sizeof(buffer), 0);
    if(bytes_read <= 0){
        if(bytes_read < 0){
            remove_client(fd);
        }
        return;
    }

    int processed = 0;
    while(processed < bytes_read){
        if(session.state == TransferState::READING_HEADER){
            size_t remaining = sizeof(session.header) - session.bytes_received_for_current_part;
            int tocopy = std::min(sizeof(bytes_read - processed), session.total_bytes_received);

            char* header_ptr = reinterpret_cast<char*>(&session.header);
            ::memcpy(header_ptr + session.bytes_received_for_current_part, buffer + processed,tocopy);

            processed += tocopy;
            session.bytes_received_for_current_part += tocopy;
            if(session.bytes_received_for_current_part == sizeof(session.header)){
                session.header.file_size = ((uint64_t)ntohl(session.header.file_size & 0xFFFFFFFF) << 32) | ntohl((session.header.file_size >> 32) & 0xFFFFFFFF);// 网络字节序转为主机字节序
                session.header.filename_len = ntohl(session.header.filename_len);

                session.bytes_received_for_current_part = 0; // 重置计数器
                session.state = TransferState::READING_FILENAME;
                
                std::cout << "Header received for client " << fd << ". File size: " << session.header.file_size 
                          << ", Filename length: " << session.header.filename_len << std::endl;
            }

        }else if(session.state == TransferState::READING_FILENAME){
            size_t remaining = session.header.filename_len - session.bytes_received_for_current_part;
            int tocopy = std::min(static_cast<size_t>(bytes_read - processed), remaining);

            session.filename.append(buffer + processed, tocopy);
            processed += tocopy;
            session.bytes_received_for_current_part += tocopy;
            if(session.header.filename_len == session.filename.length()){

                static const std::string STORAGE_DIR = "../uploads";

                auto last_slash_pos = session.filename.find_last_of("/\\");
                std::string final_filename;
                if(last_slash_pos != std::string::npos){
                    final_filename = session.filename.substr(last_slash_pos + 1);
                }else{
                    final_filename = session.filename;
                }

                session.filename = STORAGE_DIR + "/" + final_filename;

                session.bytes_received_for_current_part = 0;
                session.state == TransferState::READING_FILE_DATA;

                session.file_stream.open(session.filename, std::ios::binary);
                if(!session.file_stream){
                    std::cerr << "failed to open file ' " << session.filename << "' for writing." << std::endl;
                    remove_client(fd);
                    return;
                }

                std::cout << "Filename received for client " << fd << ": " << session.filename << std::endl;
            }
        }else if(session.state == TransferState::READING_FILE_DATA){
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

void FileServer::remove_client(int fd){
    ::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_DEL, fd, nullptr);
    pimpl_->clients.erase(fd);
    std::cout << "Client (fd=" << fd << ") disconnected.\n" << std::endl;
}

void FileServer::run(){
    std::vector<struct epoll_event> events(Impl::MAX_EVENTS);
    while(!g_shutdown){
        int nready = ::epoll_wait(pimpl_->epfd.get(), events.data(), static_cast<int>(events.size()), O_NONBLOCK);
        if(nready < 0) continue;

        for(int i = 0; i < nready; i++){
            int fd = events[i].data.fd;

            if(fd == pimpl_->server_fd.get()){
                while(true){
                    struct sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = ::accept4(pimpl_->server_fd.get(), (struct sockaddr*)&client_addr, &client_len, O_NONBLOCK);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        else {
                            std::cerr << "Accept failed. \n" << std::endl;
                            continue;
                        }
                    }

                    std::cout << "New connection (fd = " << client_fd << ")\n" << std::endl;

                    struct epoll_event ev{};
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        ::close(client_fd);
                        std::cerr << "Failed to add client to epoll_ctl()." << std::endl;
                        continue;
                    }

                    pimpl_->clients.emplace(client_addr, ClientSession{});
                }
            }else{
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                    remove_client(fd);
                    continue;
                }

                handle_client_data(fd);
            }
        }
    }
    
}