#include <thread>
#include <iostream>
#include <chrono>


void cycle_add1(){
    volatile long countA = 0;
    for(long i = 0; i < 10000000; i++){
        countA++;
    }
    std::cout << "Done!CountA =" << countA << std::endl;
}

void cycle_add2(){
    volatile long countB = 0;
    for(long j = 0; j < 10000000; j++){
        countB++;
    }
    std::cout << "Done!CountB =" << countB << std::endl;
}

void test1(){
    auto start1 = std::chrono::high_resolution_clock::now();

    std::thread t1(cycle_add1);
    std::thread t2(cycle_add2);

    t1.join();
    t2.join();

    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    std::cout << "Time: " << duration1.count() << "ms\n";
}

void test2(){
    auto start2 = std::chrono::high_resolution_clock::now();

    cycle_add1();
    cycle_add2();

    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    std::cout << "Time: " << duration2.count() << "ms\n";
}

int main(){
    test1();
    test2();
}