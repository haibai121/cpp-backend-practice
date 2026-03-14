# File Transfer Server

一个简单的基于 TCP 的文件传输服务器，用于接收客户端上传的文件。

## 项目结构
file_transfer_server/
├── bin/ # 编译生成的可执行文件存放目录
├── include/ # 头文件目录 (目前为空)
├── src/ # 源代码目录
│ ├── main.cpp # 主函数入口
│ └── server.cpp # 服务器核心逻辑实现
├── CMakeLists.txt # CMake 构建配置文件
└── README.md # 本说明文件

## 功能描述

*   **监听端口**: 服务器启动后，默认监听在 `8083` 端口。
*   **协议**: 使用自定义二进制协议进行通信。客户端首先发送一个 12 字节的协议头（包含文件大小和文件名长度），然后发送文件名，最后发送文件数据。
*   **接收文件**: 服务器接收并保存客户端上传的文件到当前工作目录下。

## 依赖

*   C++17 编译器 (如 g++, clang++)
*   CMake (版本 3.10 或更高)

## 编译与运行

### 1. 编译

```bash
# 进入项目根目录
cd file_transfer_server

# 创建构建目录
mkdir build
cd build

# 使用 CMake 配置
cmake ..

# 编译
make