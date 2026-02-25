cmake_minimum_required(VERSION 3.10)

project(chat_room
    VERSION 1.0.0
    DESCRIPTION "TCP Chat Room"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#编译警告
add_compile_options(-Wall -Wextra -Wpedantic)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#可执行文件
add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

#Linux线程库
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(server PRIVATE pthread)
    target_link_libraries(client PRIVATE pthread)
endif()

#配置信息
message(STATUS "=== TCP Chat room ===")
message(STATUS "Build: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")