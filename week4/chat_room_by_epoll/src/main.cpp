#include "chat/ChatServer.hpp"
#include "chat/Config.hpp"
#include "base/shutdown.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {

// 信号处理函数：只做 async-signal-safe 的事
void signal_handler(int sig) {
    chat::base::request_shutdown();

    // ::write 是 async-signal-safe 的
    const char msg[] = "\n[chat_server] received shutdown signal, exiting...\n";
    ::write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    (void)sig;
}

// 用 sigaction 安装，关键点：不设 SA_RESTART
void install_signal_handlers() {
    struct sigaction sa {};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // 不设 SA_RESTART：让 epoll_wait 立刻返回 EINTR

    if (::sigaction(SIGINT, &sa, nullptr) == -1) {
        throw std::runtime_error(std::string("sigaction(SIGINT) failed: ")
                                 + std::strerror(errno));
    }
    if (::sigaction(SIGTERM, &sa, nullptr) == -1) {
        throw std::runtime_error(std::string("sigaction(SIGTERM) failed: ")
                                 + std::strerror(errno));
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        install_signal_handlers();

        // 支持 ./chat_server 或 ./chat_server /path/to/server.conf
        const std::string cfg_path = (argc > 1) ? argv[1] : "config/server.conf";
        chat::ChatServer server(chat::Config::load_from_file(cfg_path));

        server.run();
        std::cout << "[chat_server] bye.\n";
    } catch (const std::exception& e) {
        std::cerr << "[chat_server] fatal: " << e.what() << '\n';
        return 1;
    }
    return 0;
}