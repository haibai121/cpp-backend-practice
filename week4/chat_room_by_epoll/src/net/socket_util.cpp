#include "net/socket_util.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chat::net {

void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error(std::string("fcntl(F_GETFL) failed: ")
                                 + std::strerror(errno));
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(std::string("fcntl(F_SETFL) failed: ")
                                 + std::strerror(errno));
    }
}

void set_reuseaddr(int fd) {
    int opt = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ")
                                 + std::strerror(errno));
    }
}

} // namespace chat::net