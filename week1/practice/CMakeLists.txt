cmake_minmum_required(VERSION 3.10)
project(ArithmeticSystem)

# 要求 c++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件，包含所有源文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(arithmetic
    main.cpp
    Question.cpp
)