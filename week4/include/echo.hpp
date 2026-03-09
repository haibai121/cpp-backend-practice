// week4/include/echo.hpp
#pragma once
#include <memory>

class EchoServer {
public:
    EchoServer();
    ~EchoServer();
    
    void run();           // 保持原有接口（内部调用 run_once）

private:
    void remove_client(int fd);

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};