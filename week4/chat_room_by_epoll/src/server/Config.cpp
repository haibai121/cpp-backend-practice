#include "chat/Config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace chat {

namespace {
std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}
} // namespace

Config Config::load_from_file(const std::string& path) {
    Config cfg;
    std::ifstream in(path);
    if (!in) return cfg;          // 没有配置文件就用默认值

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));

        try {
            if      (key == "port")             cfg.port             = static_cast<std::uint16_t>(std::stoul(val));
            else if (key == "max_events")       cfg.max_events       = std::stoul(val);
            else if (key == "max_conns")        cfg.max_conns        = std::stoul(val);
            else if (key == "listen_backlog")   cfg.listen_backlog   = std::stoi(val);
            else if (key == "recv_buffer_size") cfg.recv_buffer_size = std::stoul(val);
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid config '" + key + "': " + e.what());
        }
    }
    return cfg;
}

} // namespace chat