#pragma once

#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace base {

inline void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error(
            std::string("set_nonblocking failed: ") + std::strerror(errno));
    }
}

} // namespace base