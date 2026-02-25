# TCP Echo Server V2

>  完成时间：2026-02-18  
>  学习阶段：Week3 网络编程  
>  难度：⭐⭐

##  项目说明

支持多轮通信的 TCP 回显服务器。客户端发送什么消息，服务器原样返回。

**核心功能**：
-  客户端可连续发送多条消息
-  服务器循环接收并回显
-  输入 `/quit` 优雅退出
-  正确的 socket 资源管理

##  编译方法

```bash
# 1. 进入项目目录
cd week3/echo_v2

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置 CMake
cmake ..

# 4. 编译
make -j4

# 5. 生成的可执行文件
ls -la bin/
# server  client