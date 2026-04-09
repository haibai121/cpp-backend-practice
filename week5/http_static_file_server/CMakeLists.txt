# 指定所需的最低 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(HttpServer)

# 设置 C++ 标准为 C++17，你的代码使用了 std::make_unique 等 C++14/17 特性
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找系统库
# find_package 用于查找预装的库，对于 epoll 等 POSIX/Linux 特定 API，通常不需要显式查找。

# 定义源文件列表
# 将你的源文件添加到这里，如果有更多源文件，可以继续追加
set(SOURCES
    src/http_server.cpp
    src/main.cpp
)

# 定义头文件目录
# 将你的头文件目录添加到这里，让编译器能找到 .hpp 文件
set(HEADERS
    include/http_server.hpp
)

# 添加可执行文件
# 第一个参数是可执行文件的名称，后续是源文件列表
add_executable(http_server ${SOURCES})

# 指定头文件搜索路径
# target_include_directories 命令将 include 目录添加到 http_server 目标的编译选项中
target_include_directories(http_server PRIVATE include)

# 链接库
# 你的代码使用了 socket 和 epoll 等系统调用，需要链接系统库
# 在 Linux 上，通常需要链接 pthread 和 rt (real-time) 库
target_link_libraries(http_server PRIVATE pthread rt)

# （可选）设置编译选项，例如启用更多警告
target_compile_options(http_server PRIVATE -Wall -Wextra -pedantic)