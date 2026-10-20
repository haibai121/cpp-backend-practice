#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace chat {

struct Config {
    std::uint16_t port{8082};
    std::size_t   max_events{1024};
    std::size_t   max_conns{1024};
    int           listen_backlog{10};
    std::size_t   recv_buffer_size{4096};

    static Config load_from_file(const std::string& path);
};

} // namespace chat