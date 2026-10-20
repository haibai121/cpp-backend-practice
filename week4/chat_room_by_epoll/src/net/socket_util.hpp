#pragma once

namespace chat::net {
void set_nonblocking(int fd);
void set_reuseaddr(int fd);
}