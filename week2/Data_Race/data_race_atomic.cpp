#include <iostream>
#include <thread>
#include <atomic>

std::atomic<long> counter{0};

void add_counter1(){
    for(long i = 0; i < 100000000; i++){
        counter++;
    }
    std::cout << "Done!\n";
}

void add_counter2(){
    for(long j = 0; j < 100000000; j++){
        counter++;
    }
    std::cout << "Done!\n";
}

int main(){
    std::thread t1(add_counter1);
    std::thread t2(add_counter2);

    t1.join();
    t2.join();

    std::cout << "All Done! counter = " << counter ;

}