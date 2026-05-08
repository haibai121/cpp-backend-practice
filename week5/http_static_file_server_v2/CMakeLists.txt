cmake_minimum_required(VERSION 3.10)

project(Static_Http_Server_V2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES 
    src/Http_server.cpp
    src/main.cpp
)

set(HEADERS 
    include/Http_server.hpp
)

add_executable(http_server_v2 ${SOURCES})

target_include_directories(http_server_v2 PRIVATE include)

target_link_libraries(http_server_v2 PRIVATE pthread rt)