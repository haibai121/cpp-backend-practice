#pragma once
#include<memory>

class FileServer{
public:
    FileServer();
    ~FileServer();
    

    void run();
private:
    void remove_client(int fd);
    void handle_client_data(int fd);
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};