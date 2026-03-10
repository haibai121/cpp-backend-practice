# 时间服务器

## 项目简介

一个基于 epoll 的高性能时间服务器，当客户端连接后，服务器会返回当前的时间信息。

## 功能特性

- **实时时间服务**：客户端连接后立即返回当前服务器时间
- **高性能 I/O**：使用 epoll 事件驱动模型，支持大量并发连接
- **自动资源管理**：使用 RAII 模式自动管理 socket 资源

## 技术难点

- **epoll 基本用法**：掌握 epoll_create1、epoll_ctl、epoll_wait 的使用
- **非阻塞 I/O**：使用 accept4 设置非阻塞 socket，避免线程阻塞
- **事件驱动编程**：基于事件循环处理连接和数据读写
- **资源管理**：使用 RAII 模式确保 socket 和 epoll 句柄的正确释放

## 系统要求

- Linux 系统（支持 epoll）
- C++11 或更高版本
- g++ 或 clang++ 编译器
- CMake 3.10 或更高版本

## 编译运行

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make

# 运行（端口9090）
./time_server

# 或指定端口
./time_server 9090