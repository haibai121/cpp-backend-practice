#include "/home/cppdev/projects/cpp-backend-practice/week4/chat_room_by_epoll/include/Echo.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include <fcntl.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <cstring> // for strerror
#include <cerrno>  // for errno

extern bool g_shutdown;

namespace{
    void set_nonblocking(int fd){
        int flat = ::fcntl(fd, F_GETFL, 0);
        if(flat == -1 || ::fcntl(fd, F_SETFL,flat | FNONBLOCK)){
            throw std::runtime_error("设置非阻塞失败");
        }
    }


    std::string get_currenttime(){
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }

    class FileDescriptor{
        int fd_;
    public:
        explicit FileDescriptor(int fd = -1) : fd_(fd) {};
        ~FileDescriptor(){if(fd_ >= 0) ::close(fd_);};

        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_){ 
            other.fd_ = -1; 
        }
        FileDescriptor& operator=(FileDescriptor&& other) noexcept {
            if(this != &other){
                if (fd_ >= 0) ::close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const { return fd_; }
        explicit operator bool() const {return fd_ >= 0;}
    };
}

struct EchoServer::Impl{
    FileDescriptor server_fd;
    FileDescriptor epfd;
    std::unordered_map<int, FileDescriptor> clients;
    std::vector<int> fds;
    static constexpr int PORT = 8082;
    static constexpr size_t MAX_EVENTS = 1024;
};

EchoServer::EchoServer() : pimpl_(std::make_unique<Impl>()){
    pimpl_->server_fd = FileDescriptor(::socket(AF_INET, SOCK_STREAM, 0));
    if (!pimpl_->server_fd) {
        throw std::runtime_error("socket() failed");
    }

    int opt = 1;
    ::setsockopt(pimpl_->server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(Impl::PORT);

    if(::bind(pimpl_->server_fd.get(), (struct sockaddr*)&address, sizeof(address)) == -1){
        perror("Bind failed"); 
        throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));
        strerror(errno);
    }
    if(::listen(pimpl_->server_fd.get(), 10) == -1){
        throw std::runtime_error("listen() failed");
    }

    set_nonblocking(pimpl_->server_fd.get());
    //这句话刚刚没加FileDescriptor
    pimpl_->epfd = FileDescriptor(::epoll_create1(EPOLL_CLOEXEC));
    if (!pimpl_->epfd) {
        throw std::runtime_error("epoll_create1() failed");
    }

    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = pimpl_->server_fd.get();
    if(::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, pimpl_->server_fd.get(), &ev) == -1){
    throw std::runtime_error("epoll_ctl(ADD server_fd) failed");
    }

    std::cout << "Echo server listening on port " << pimpl_->PORT << "\n";
}


EchoServer::~EchoServer() = default;

void EchoServer::run(){
    std::vector<struct epoll_event> events(pimpl_->MAX_EVENTS);

    while(!g_shutdown){
        int nready = epoll_wait(pimpl_->epfd.get(), events.data(), static_cast<int>(events.size()), 5000);
        if(nready <= 0) continue;

        for(int i = 0; i < nready;i++){
            int fd = events[i].data.fd;

            if(fd == pimpl_->server_fd.get()){
                while(true){
                    struct sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = ::accept4(
                        pimpl_->server_fd.get(),
                        (struct sockaddr*)&client_addr,
                        &client_len,
                        SOCK_NONBLOCK
                    );

                    if(client_fd == -1){
                        if(errno == EAGAIN || errno == EWOULDBLOCK)break;
                        else{
                            std::cerr << "Accept error\n";
                            break;
                        }
                    }

                    std::cout << "New client connected (fd=" << client_fd << ")\n";

                    struct epoll_event ev{};
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if(::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, client_fd, &ev) == -1){
                        ::close(client_fd);
                        std::cerr << "Failed to add client to epoll\n";
                        continue;
                    }

                    pimpl_->clients.emplace(client_fd, FileDescriptor(client_fd));
                    pimpl_->fds.emplace_back(client_fd);
                }
            }else{
                    if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                        remove_client(fd);
                        continue;
                    }
                    char buffer[4096] = {0};
                    size_t bytes_read = ::recv(fd, buffer, sizeof(buffer)-1, 0);
                    
                    if(bytes_read <= 0){
                        std::cout << "Client (fd=" << fd << ") disconnected (recv returned " << bytes_read << ").\n";
                        remove_client(fd);
                        continue;
                    }

                    std::string message(buffer, bytes_read);

                    if(message == "quit\n"){
                        std::cout << "Client (fd= " << fd << ") send 'quit', disconnecting.\n";
                        remove_client(fd);
                        continue;
                    }

                    for(int fd_ : pimpl_->fds ){
                        if(pimpl_->fds[fd_] == fd){
                            continue;
                        }

                        std::string time = get_currenttime();
                        std::string full_message = "[" + time + "]" + (std::string)buffer + "\0";

                        size_t bytes_send = ::send(fd_, full_message.c_str(), full_message.length(), 0);
                        if(bytes_send == -1){
                            std::cerr << "Send error to client (fd=" << fd << "): " << strerror(errno) << "\n";
                            remove_client(fd);
                        }
                    }
                }
        }
    }
        
}


void EchoServer::remove_client(int client_fd){
    std::cout << "fd = " << client_fd << " disconnection." << std::endl;
    ::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_DEL, client_fd, nullptr);
    pimpl_->clients.erase(client_fd);
    for(auto it = pimpl_->fds.begin();it != pimpl_->fds.end(); it++){
        if(*it == client_fd){
            pimpl_->fds.erase(it);
            break;
        }
    }
    //
}