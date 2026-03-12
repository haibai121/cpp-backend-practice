#pragma once
#include <iostream>
#include <memory>

class EchoServer{
public:
    EchoServer();
    ~EchoServer();

    void run();

private:
    void remove_client(int fd);
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};