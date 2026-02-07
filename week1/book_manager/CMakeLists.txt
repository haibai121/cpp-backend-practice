cmake_minimum_required(VERSION 3.10)
project(BookManager)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)

add_executable(book_manager
    main.cpp
    # 后续可以添加 Student.cpp、StudentManager.cpp 等
)