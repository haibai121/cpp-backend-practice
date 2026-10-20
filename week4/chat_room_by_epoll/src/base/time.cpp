#include "base/time.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace chat::base {

std::string current_time_string() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
    localtime_r(&tt, &tm_buf);            // 线程安全版 localtime

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace chat::base