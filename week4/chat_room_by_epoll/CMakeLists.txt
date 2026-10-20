cmake_minimum_required(VERSION 3.16)
project(chat_room_by_epoll LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB_RECURSE SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/base/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/server/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/session/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/business/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/*.cpp
)

add_executable(chat_server
  ${SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_include_directories(chat_server PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(chat_server PRIVATE _GNU_SOURCE)