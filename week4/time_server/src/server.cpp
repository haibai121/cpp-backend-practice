#include "/home/cppdev/projects/cpp-backend-practice/week4/time_server/include/echo.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

extern bool g_shutdown;

namespace{
    void set_nonblocking(int fd){
        int flag = ::fcntl(fd, F_GETFL, 0);
        if(flag == -1 || ::fcntl(fd, F_SETFL, flag | FNONBLOCK)){
            std::runtime_error("Failed to set nonblocking");
        }
    }

    std::string get_currenttime(){
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }

    class FileDescript{
        int fd_;
        public:
        FileDescript(int fd = -1) : fd_(fd){}
        ~FileDescript(){ if(fd_ >= 0) ::close(fd_); }

        FileDescript(const FileDescript&) = delete;
        FileDescript& operator=(const FileDescript&) = delete;

        FileDescript(FileDescript&& other) noexcept : fd_(other.fd_){ 
            other.fd_ = -1;  
        }
        FileDescript& operator=(FileDescript&& other) noexcept{
            if(this != &other){
                if(fd_ >= 0) ::close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }

            return *this;
        }

        int get() const {return fd_;}
        explicit operator bool() const {return fd_ >= 0;}
    };
}

struct EchoTimeServer::Impl{
FileDescript server_fd;
FileDescript epfd;
std::unordered_map<int, FileDescript> clients;
static constexpr int PORT = 9090;
static constexpr int MAX_EVENTS = 1024;
};

EchoTimeServer::EchoTimeServer() : pimpl_(std::make_unique<Impl>()){
pimpl_->server_fd = FileDescript(socket(AF_INET, SOCK_STREAM, 0));
if (!pimpl_->server_fd) {
        throw std::runtime_error("socket() failed");
    }

int opt = 1;
::setsockopt(pimpl_->server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

struct sockaddr_in address{};
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(pimpl_->PORT);

if((::bind(pimpl_->server_fd.get(), (struct sockaddr*)&address, sizeof(address)) == -1 || 
    ::listen(pimpl_->server_fd.get(), 10)) == -1){
    throw std::runtime_error("bind() or listen() failed");
}

set_nonblocking(pimpl_->server_fd.get());

pimpl_->epfd = FileDescript(::epoll_create1(EPOLL_CLOEXEC));
if(!pimpl_->epfd){
    throw std::runtime_error("epoll_create1() failed");
}

struct epoll_event ev{};
ev.events = EPOLLIN;
ev.data.fd = pimpl_->server_fd.get();
if(::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, pimpl_->server_fd.get(), &ev) == -1){
    throw std::runtime_error("epoll_ctl() failed");
}

std::cout << "Echo server listening on port " << Impl::PORT << "\n";

};

EchoTimeServer::~EchoTimeServer() = default;

void EchoTimeServer::run(){
    std::vector<struct epoll_event> events(pimpl_->MAX_EVENTS);
    
    while(!g_shutdown){
        int nready = ::epoll_wait(pimpl_->epfd.get(), events.data(), pimpl_->MAX_EVENTS, 5000);
        if(nready <= 0) continue;

        for(int i = 0;i < nready; ++i){
            int fd = events[i].data.fd;

            if(fd == pimpl_->server_fd.get()){
                while(true){
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept4(pimpl_->server_fd.get(), 
                    (struct sockaddr*)&client_addr, 
                    &client_len, 
                    SOCK_NONBLOCK);

                    if(client_fd == -1){
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            else {
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

                    pimpl_->clients.emplace(client_fd, FileDescript(client_fd));
                }
            }else{
                if(events[i].events & (EPOLLHUP | EPOLLRDHUP)){
                    remove_client(fd);
                    continue;
                }

                char buffer[4096] = {0};
                std::string time = get_currenttime();
                int nread = ::recv(fd, buffer, sizeof(buffer)-1, 0);
                if(nread <= 0){
                    remove_client(fd);
                }else{
                    buffer[nread] = '\0';
                    std::string full_message = "[" + time + "]:" + std::string(buffer);
                    ::send(fd, full_message.c_str(), full_message.length(), 0);
                }
            }
        }
    }
}

void EchoTimeServer::remove_client(int fd){
    std::cout << "Client disconnected (fd=" << fd << ")\n";
    ::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_DEL, fd, nullptr);
    pimpl_->clients.erase(fd);
}