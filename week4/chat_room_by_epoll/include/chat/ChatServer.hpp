#pragma once

#include "chat/Config.hpp"

#include <memory>

namespace chat {

class ChatServer {
public:
    ChatServer();
    explicit ChatServer(Config cfg);
    ~ChatServer();

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    void run();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace chat