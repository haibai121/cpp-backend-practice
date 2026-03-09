#include "echo.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <csignal> // 确保能访问全局 g_shutdown

// ============= 辅助函数 =============
extern bool g_shutdown;

namespace {
    void set_nonblocking(int fd) {
        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Failed to set nonblocking");
        }
    }

    class FileDescriptor {
        int fd_;
    public:
        explicit FileDescriptor(int fd = -1) : fd_(fd) {}
        ~FileDescriptor() { if (fd_ >= 0) ::close(fd_); }

        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }
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
}

// ============= EchoServer 实现 =============

struct EchoServer::Impl {
    FileDescriptor server_fd;
    FileDescriptor epfd;
    std::unordered_map<int, FileDescriptor> clients;
    static constexpr int PORT = 8888;
    static constexpr size_t MAX_EVENTS = 1024;
};

EchoServer::EchoServer() : pimpl_(std::make_unique<Impl>()) {
    // 创建监听 socket
    pimpl_->server_fd = FileDescriptor(::socket(AF_INET, SOCK_STREAM, 0));
    if (!pimpl_->server_fd) {
        throw std::runtime_error("socket() failed");
    }

    int opt = 1;
    ::setsockopt(pimpl_->server_fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Impl::PORT);

    if (::bind(pimpl_->server_fd.get(), (struct sockaddr*)&addr, sizeof(addr)) == -1 ||
        ::listen(pimpl_->server_fd.get(), 10) == -1) {
        throw std::runtime_error("bind() or listen() failed");
    }

    set_nonblocking(pimpl_->server_fd.get());

    // 创建 epoll
    pimpl_->epfd = FileDescriptor(::epoll_create1(EPOLL_CLOEXEC));
    if (!pimpl_->epfd) {
        throw std::runtime_error("epoll_create1() failed");
    }

    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = pimpl_->server_fd.get();
    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, pimpl_->server_fd.get(), &ev) == -1) {
        throw std::runtime_error("epoll_ctl(ADD server_fd) failed");
    }

    std::cout << "Echo server listening on port " << Impl::PORT << "\n";
}

EchoServer::~EchoServer() = default;

void EchoServer::run() {
    std::vector<struct epoll_event> events(Impl::MAX_EVENTS);

    while (!g_shutdown) {
        int nready = ::epoll_wait(pimpl_->epfd.get(), events.data(), static_cast<int>(events.size()), 500);
        if (nready <= 0) continue; // 被信号中断

        for (int i = 0; i < nready; ++i) {
            int fd = events[i].data.fd;

            if (fd == pimpl_->server_fd.get()) {
                // 处理新连接
                while (true) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = ::accept4(
                        pimpl_->server_fd.get(),
                        (struct sockaddr*)&client_addr,
                        &client_len,
                        SOCK_NONBLOCK
                    );

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        else {
                            std::cerr << "Accept error\n";
                            break;
                        }
                    }

                    std::cout << "New client connected (fd=" << client_fd << ")\n";

                    // 加入 epoll 监控
                    struct epoll_event ev{};
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        ::close(client_fd);
                        std::cerr << "Failed to add client to epoll\n";
                        continue;
                    }

                    // 保存客户端（RAII 自动管理）
                    pimpl_->clients.emplace(client_fd, FileDescriptor(client_fd));
                }
            } else {
                // 处理客户端数据
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                    remove_client(fd);
                    continue;
                }

                char buffer[4096];
                ssize_t nread = ::read(fd, buffer, sizeof(buffer));
                if (nread <= 0) {
                    remove_client(fd);
                } else {
                    // Echo 回去
                    ::write(fd, buffer, nread);
                }
            }
        }
    }
}

void EchoServer::remove_client(int fd) {
    std::cout << "Client disconnected (fd=" << fd << ")\n";
    ::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_DEL, fd, nullptr);
    pimpl_->clients.erase(fd); // RAII 自动 close
}