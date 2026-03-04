CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

project(chat_room_v2
    VERSION 1.0.0
    DESCRIPTION "TCP Multi Client Chat Room"
    LANGUAGE CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)