#include "base/TimeUtil.hpp"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace base {

std::string get_currenttime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
    ::localtime_r(&t, &tm_buf);

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace base