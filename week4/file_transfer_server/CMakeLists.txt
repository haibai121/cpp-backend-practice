cmake_minimum_required(VERSION 3.10)

# 1. 指定所需的 CMake 最低版本
cmake_minimum_required(VERSION 3.10)

# 2. 定义项目名称和版本
project(file_transfer_server VERSION 1.0)

# 3. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 设置编译器选项 (可选，但推荐)
# -Wall: 启用所有警告
# -Wextra: 启用额外警告
# -g: 生成调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 5. 查找系统库 (如果需要)
# find_package(Threads REQUIRED) # 如果项目使用了线程

# 6. 定义源文件列表
# 将所有 .cpp 文件添加到变量中
set(SOURCES
    src/server.cpp
    src/main.cpp # 假设你的主实现文件在这里
    # 如果有其他 .cpp 文件，也在这里添加，例如：
    # src/utils.cpp
)

# 7. 定义头文件目录
# 告诉编译器在哪里寻找 #include <file.hpp>
include_directories(${PROJECT_SOURCE_DIR}/include)

# 8. 添加可执行文件
# 第一个参数是最终生成的可执行文件名
# 后面跟着源文件列表
add_executable(server ${SOURCES})

# 9. 链接库 (如果需要)
# target_link_libraries(file_transfer_server Threads::Threads)
# 对于你当前的服务器代码，可能不需要链接额外的库，因为用的是系统调用。

# 10. (可选) 安装规则
install(TARGETS server DESTINATION bin)