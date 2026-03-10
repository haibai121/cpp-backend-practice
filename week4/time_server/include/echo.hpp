#pragma once
#include <memory>

class EchoTimeServer{
public:
    EchoTimeServer();
    ~EchoTimeServer();

    void run();
private:
    void remove_client(int fd);
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};