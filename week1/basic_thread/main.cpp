#include <iostream>
#include <thread>
#include <string>

void print_message(std::string msg, int count){
    for(int i; i < count; i++){
        std::cout << msg << "(" << i << ")" << std::endl;
    }
}

int main(){
    std::thread t(print_message, "你好世界！", 5);

    print_message("完美世界！", 5);

    t.join();

    std::cout << "全部完成" << std::endl;
}