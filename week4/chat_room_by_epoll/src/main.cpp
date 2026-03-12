#include "/home/cppdev/projects/cpp-backend-practice/week4/chat_room_by_epoll/include/Echo.hpp"
#include<iostream>
#include<chrono>
#include <csignal>
#include <cstdlib> 

bool g_shutdown = false;

void signal_handler(int sig) {
    g_shutdown = true;
    std::cout << "\nReceived signal " << sig << ". Shutting down...\n";
}


int main(){
    std::signal(SIGINT,  signal_handler);   
    std::signal(SIGTERM, signal_handler);
    try{
        EchoServer server;

        server.run();
    }catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}