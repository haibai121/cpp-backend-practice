cmake_minimum_required(VERSION 3.10)

project(chat_room_v3
    LANGUAGES CXX
    VERSION 3.0.0
    DESCRIPTION "TCP Multi Client Chat Room"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  

find_package(Threads REQUIRED)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)