#include "base/shutdown.hpp"

namespace chat::base {
volatile std::sig_atomic_t g_shutdown = 0;
}