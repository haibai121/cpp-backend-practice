#pragma once
#include <iostream>

class BroadCastServer{
public:
    BroadCastServer();
    ~BroadCastServer();
    void broadcast(int client_fd, int epfd);
};