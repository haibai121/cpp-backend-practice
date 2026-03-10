# week4/epoll_echo_server/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(EchoServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集所有源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(server ${SOURCES})

# 设置头文件目录
target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/include)

# 设置输出目录
set_target_properties(server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)