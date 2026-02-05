cmake_minimum_required(VERSION 3.10)
project(StudentManager)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)

# 添加可执行程序
add_executable(student_manager
    main.cpp
    # 后续可以添加 Student.cpp、StudentManager.cpp 等
)