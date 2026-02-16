#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

std::queue<int> buffer;
std::mutex mtx;
std::condition_variable cv;
int MAX_SIZE = 5;
int MAX_ITEM = 20;

void consumer(){
    int i = 0;
    while(i < MAX_ITEM){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{return !buffer.empty();});

        int val = buffer.front();

        buffer.pop();

        std::cout << "消费了：" << val << "\n";

        lock.unlock();

        i++;
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

void producer(){
    int item = 1;
    while(item < MAX_ITEM){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{return buffer.size() < MAX_SIZE;});
        buffer.push(item);
        std::cout << "生产了：" << item <<"\n";
        item++;

        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(){
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();

    std::cout << "All done!";
}