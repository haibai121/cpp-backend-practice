#pragma once
#include <csignal>

namespace chat::base {

extern volatile std::sig_atomic_t g_shutdown;

inline void request_shutdown() noexcept          { g_shutdown = 1; }
inline bool is_shutdown_requested() noexcept     { return g_shutdown != 0; }

}