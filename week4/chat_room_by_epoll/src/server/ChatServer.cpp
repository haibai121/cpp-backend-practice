#include "chat/ChatServer.hpp"

#include "base/FileDescriptor.hpp"
#include "base/shutdown.hpp"
#include "base/time.hpp"
#include "net/socket_util.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace chat {

struct ChatServer::Impl {
    explicit Impl(Config c) : cfg(std::move(c)) {}

    Config cfg;
    base::FileDescriptor server_fd;
    base::FileDescriptor epfd;
    std::unordered_map<int, base::FileDescriptor> clients;
    std::vector<int> fds;

    void accept_loop();
    void handle_client(int fd);
    void broadcast(int from_fd, const std::string& message);
    void remove_client(int client_fd);
};

// ---------- 事件处理 ----------

void ChatServer::Impl::accept_loop() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = ::accept4(
            server_fd.get(),
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_len,
            SOCK_NONBLOCK);

        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::cerr << "Accept error: " << std::strerror(errno) << '\n';
            break;
        }

        std::cout << "New client connected (fd=" << client_fd << ")\n";

        epoll_event ev{};
        ev.events  = EPOLLIN;
        ev.data.fd = client_fd;
        if (::epoll_ctl(epfd.get(), EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            std::cerr << "Failed to add client to epoll: "
                      << std::strerror(errno) << '\n';
            ::close(client_fd);
            continue;
        }

        clients.emplace(client_fd, base::FileDescriptor(client_fd));
        fds.push_back(client_fd);
    }
}

void ChatServer::Impl::handle_client(int fd) {
    char buffer[4096] = {0};
    ssize_t n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (n <= 0) {
        std::cout << "Client (fd=" << fd << ") disconnected (recv=" << n << ").\n";
        remove_client(fd);
        return;
    }

    std::string message(buffer, static_cast<std::size_t>(n));

    if (message == "quit\n") {
        std::cout << "Client (fd=" << fd << ") sent 'quit', disconnecting.\n";
        remove_client(fd);
        return;
    }

    // 广播时拼接时间戳
    broadcast(fd, "[" + base::current_time_string() + "] " + message);
}

void ChatServer::Impl::broadcast(int from_fd, const std::string& message) {
    std::vector<int> failed;   // 先收集再删，避免迭代时修改
    for (int fd : fds) {
        if (fd == from_fd) continue;
        if (::send(fd, message.data(), message.size(), 0) == -1) {
            std::cerr << "Send to fd=" << fd << " failed: "
                      << std::strerror(errno) << '\n';
            failed.push_back(fd);
        }
    }
    for (int fd : failed) remove_client(fd);
}

void ChatServer::Impl::remove_client(int client_fd) {
    std::cout << "fd = " << client_fd << " disconnected.\n";
    ::epoll_ctl(epfd.get(), EPOLL_CTL_DEL, client_fd, nullptr);
    clients.erase(client_fd);
    for (auto it = fds.begin(); it != fds.end(); ++it) {
        if (*it == client_fd) { fds.erase(it); break; }
    }
    // FileDescriptor 的析构函数负责 close
}

// ---------- ChatServer 构造 / 运行 ----------

ChatServer::ChatServer() : ChatServer(Config::load_from_file("config/server.conf")) {}

ChatServer::ChatServer(Config cfg)
    : pimpl_(std::make_unique<Impl>(std::move(cfg))) {

    pimpl_->server_fd = base::FileDescriptor(::socket(AF_INET, SOCK_STREAM, 0));
    if (!pimpl_->server_fd) throw std::runtime_error("socket() failed");

    net::set_reuseaddr(pimpl_->server_fd.get());

    sockaddr_in address{};
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(pimpl_->cfg.port);

    if (::bind(pimpl_->server_fd.get(),
               reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) == -1) {
        throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno));
    }
    if (::listen(pimpl_->server_fd.get(), pimpl_->cfg.listen_backlog) == -1) {
        throw std::runtime_error(std::string("listen() failed: ") + std::strerror(errno));
    }

    net::set_nonblocking(pimpl_->server_fd.get());

    pimpl_->epfd = base::FileDescriptor(::epoll_create1(EPOLL_CLOEXEC));
    if (!pimpl_->epfd) throw std::runtime_error("epoll_create1() failed");

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = pimpl_->server_fd.get();
    if (::epoll_ctl(pimpl_->epfd.get(), EPOLL_CTL_ADD,
                    pimpl_->server_fd.get(), &ev) == -1) {
        throw std::runtime_error(std::string("epoll_ctl(ADD server_fd) failed: ")
                                 + std::strerror(errno));
    }

    std::cout << "Chat server listening on port " << pimpl_->cfg.port << "\n";
}

ChatServer::~ChatServer() = default;

void ChatServer::run() {
    std::vector<epoll_event> events(pimpl_->cfg.max_events);

    while (!base::is_shutdown_requested()) {
        int n = ::epoll_wait(pimpl_->epfd.get(), events.data(),
                             static_cast<int>(events.size()), 5000);

        if (n < 0) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error: " << std::strerror(errno) << '\n';
            break;
        }
        if (n == 0) continue;

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == pimpl_->server_fd.get()) {
                pimpl_->accept_loop();
                continue;
            }
            if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                pimpl_->remove_client(fd);
                continue;
            }
            if (events[i].events & EPOLLIN) {
                pimpl_->handle_client(fd);
            }
        }
    }
}

void ChatServer::stop() { base::request_shutdown(); }

} // namespace chat