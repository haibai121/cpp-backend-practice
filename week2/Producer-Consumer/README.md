# Producer-Consumer Model

A classic thread synchronization implementation in C++ using mutex and condition variable.

## 功能特性

- **共享有界缓冲区**：使用 `std::queue<int>` 作为 FIFO 缓冲区，最大容量为 5。
- **线程安全**：通过 `std::mutex` 保护所有对共享缓冲区的访问。
- **高效等待**：
  - 生产者在缓冲区满时阻塞等待（不忙等待）
  - 消费者在缓冲区空时阻塞等待
- **精确控制**：生产并消费 20 个整数（1 到 20），程序自动退出。

## 编译与运行

```bash
# 编译（要求 C++17 或更高）
c++ -std=c++17 -pthread -O0 main.cpp -o pc

# 运行
./pc