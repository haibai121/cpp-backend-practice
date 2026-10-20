# chat_room_by_epoll

基于 Linux epoll 的高性能多人在线聊天室，采用主从 Reactor + 线程池架构，支持自定义二进制协议、异步日志、时间轮心跳、离线消息持久化，并附带完整的压测工具与数据。

## 目录

- [项目特性](#项目特性)
- [架构设计](#架构设计)
- [目录结构](#目录结构)
- [通信协议](#通信协议)
- [快速开始](#快速开始)
- [配置说明](#配置说明)
- [压测数据](#压测数据)
- [核心技术点](#核心技术点)
- [开发路线](#开发路线)
- [文档索引](#文档索引)

## 项目特性

### 网络与并发

- 主从 Reactor 模型：主线程只负责 accept，工作线程池处理读写与业务，充分利用多核 CPU
- epoll 边缘触发（ET）+ 非阻塞 IO：单线程高效管理上万并发连接
- RAII 资源管理：FileDescriptor 封装 fd，异常退出自动回收，杜绝资源泄漏
- 线程安全：互斥锁 + 条件变量保护在线用户列表、消息队列等共享数据

### 协议与业务

- 自定义二进制协议：定长包头 + 变长包体，解决 TCP 粘包/拆包
- 用户系统：注册、登录、登出
- 聊天功能：群聊广播、私聊、多房间
- 在线状态：在线用户列表同步、用户上下线系统通知
- 离线消息：用户不在线时消息持久化，上线后拉取

### 可靠性

- 心跳保活：基于分层时间轮，O(1) 检测超时连接并自动清理
- 异步日志：muduo 风格双缓冲，前端仅 memcpy，后端批量落盘
- 统一错误码：所有响应携带错误码，便于客户端处理
- 异常重连：客户端断线后支持重新登录恢复状态

### 性能

- 自写多线程压测客户端，支持连接数/吞吐/延迟多维压测
- 实测 10k 并发连接、38.5 万 QPS、P99 延迟小于 1ms
- 异步日志相对同步日志 QPS 提升 7.7 倍

## 架构设计

### 主从 Reactor 模型

```
                    +---------------------+
                    |   MainReactor       |  <- 主线程，只负责 accept
                    |  (main thread)      |
                    |  listen_fd + epoll  |
                    +----------+----------+
                               |
                               | 新连接 client_fd
                               | round-robin 分发
              +----------------+----------------+
              |                |                |
              v                v                v
      +--------------+ +--------------+ +--------------+
      | SubReactor 0 | | SubReactor 1 | | SubReactor N |  <- 线程池
      |  epoll + 循环| |  epoll + 循环| |  epoll + 循环|
      +--------------+ +--------------+ +--------------+
              |                |                |
              +----------------+----------------+
                               |
                               v
                    +---------------------+
                    |  EventLoopThreadPool|
                    |  (管理 N 个线程)     |
                    +---------------------+
```

### 分层设计

| 层次 | 职责 |
|------|------|
| base | 基础工具：RAII、日志、时间轮、错误码 |
| net | 网络核心：EventLoop、Channel、EpollPoller、线程池 |
| protocol | 协议：Packet、编解码、粘包处理 |
| session | 会话：连接管理、心跳 |
| business | 业务：用户、群聊、私聊、消息分发 |
| storage | 持久化：离线消息、SQLite |
| server | 编排：ChatServer 组装所有模块 |

## 目录结构

```
chat_room_by_epoll/
├── CMakeLists.txt
├── README.md
├── config/
│   └── server.conf
│
├── include/chat/              # 对外 API
│   ├── ChatServer.hpp
│   └── Config.hpp
│
├── src/
│   ├── main.cpp
│   ├── server/                # 编排层
│   ├── base/                  # 基础工具
│   │   ├── log/               # 异步日志
│   │   ├── timer/             # 时间轮
│   │   └── memory/            # 对象池
│   ├── net/                   # 网络层
│   ├── protocol/              # 协议层
│   ├── session/               # 会话层
│   ├── business/              # 业务层
│   └── storage/               # 持久化
│
├── benchmark/                 # 压测
│   ├── client/
│   ├── scripts/
│   └── results/
│
├── tests/                     # 单元测试
└── docs/                      # 文档
```

## 通信协议

采用定长包头 + 变长包体的二进制协议，解决 TCP 粘包/拆包。

### 包头结构（12 字节）

```
+--------+---------+-------+--------+--------+
| magic  | version | type  | length |  seq   |
|  2B    |   1B    |  1B   |   4B   |   4B   |
+--------+---------+-------+--------+--------+
```

| 字段 | 说明 |
|------|------|
| magic | 固定 0xCAFE，快速校验 |
| version | 协议版本号 |
| type | 消息类型 |
| length | 包体长度（网络字节序） |
| seq | 序列号，用于请求响应匹配 |

### 消息类型

| 类型 | 说明 |
|------|------|
| HEARTBEAT_REQ/RSP | 心跳 |
| REGISTER_REQ/RSP | 注册 |
| LOGIN_REQ/RSP | 登录 |
| LOGOUT_REQ/RSP | 登出 |
| CHAT_MSG | 群聊消息 |
| PRIVATE_MSG | 私聊消息 |
| USER_LIST_REQ/RSP | 在线列表 |
| USER_JOIN/LEAVE_NTF | 上下线通知 |
| SYSTEM_NOTIFY | 系统消息 |
| ERROR_RSP | 错误响应 |

## 快速开始

### 环境要求

- Linux（内核 3.9+，支持 epoll、accept4、timerfd）
- CMake 3.16+
- GCC 9+ 或 Clang 10+
- C++17

### 编译

```bash
git clone <repo-url>
cd chat_room_by_epoll
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行服务端

```bash
./chat_server ../config/server.conf
```

默认监听 0.0.0.0:8082。

### 运行测试客户端

```bash
./test_client 127.0.0.1 8082
```

### 运行单元测试

```bash
cd build
ctest --output-on-failure
```

## 配置说明

config/server.conf：

```ini
# 监听端口
port = 8082

# IO 线程数（SubReactor 数量），建议 = CPU 核数
io_threads = 8

# 心跳间隔（秒），客户端应在此时间内发包
heartbeat_interval = 30

# 心跳超时（秒），超过则断开连接
heartbeat_timeout = 90

# 日志级别：TRACE / DEBUG / INFO / WARN / ERROR / FATAL
log_level = INFO

# 日志文件路径
log_path = ./logs/chat_server.log

# 日志滚动大小（MB）
log_roll_size = 100

# 离线消息数据库路径
db_path = ./data/chat.db

# 单连接输入缓冲区上限（KB），超过则断开
max_input_buffer = 1024
```

## 压测数据

### 测试环境

| 项 | 配置 |
|----|------|
| CPU | Intel i7-12700 (12C20T) |
| 内存 | 32GB DDR4 |
| OS | Ubuntu 22.04, kernel 5.15 |
| 编译 | g++ 11.3, -O2 |
| 服务端线程 | 1 main + 8 sub |

### 1. 最大并发连接数

| 连接数 | 建立耗时 | 内存(RSS) | 线程数 | fd 数 | 成功率 |
|--------|---------|-----------|--------|-------|--------|
| 1,000  | 0.8s    | 12 MB     | 9      | 1010  | 100%   |
| 5,000  | 3.2s    | 38 MB     | 9      | 5010  | 100%   |
| 10,000 | 6.5s    | 71 MB     | 9      | 10010 | 100%   |
| 20,000 | 14.1s   | 142 MB    | 9      | 20010 | 100%   |

### 2. 消息吞吐（1k 连接，64B 消息）

| 指标 | 值 |
|------|-----|
| QPS | 385,000 |
| 吞吐 | 23.5 MB/s |
| CPU（服务端） | 620% |
| CPU（客户端） | 480% |
| 内存 | 45 MB |

### 3. 延迟分布（100 连接，限速 100 msg/s）

| 指标 | 值 |
|------|-----|
| avg | 0.18 ms |
| P50 | 0.15 ms |
| P90 | 0.32 ms |
| P99 | 0.87 ms |
| P999 | 2.10 ms |
| max | 8.40 ms |

### 4. 异步日志影响

| 场景 | QPS | CPU |
|------|-----|-----|
| 关闭日志 | 385,000 | 620% |
| 异步日志 INFO | 372,000 | 640% |
| 同步日志 INFO | 48,000 | 580% |

结论：异步日志相对同步日志提升 7.7 倍，相对无日志仅损失 3.4%。

### 复现压测

```bash
# 终端 1：启动服务端并监控
./chat_server ../config/server.conf
./benchmark/scripts/monitor.sh $(pidof chat_server)

# 终端 2：运行压测
./bench_client --host 127.0.0.1 --port 8082 \
               --connections 1000 --threads 4 \
               --duration 60 --msg-size 64
```

## 核心技术点

### 1. 主从 Reactor + 线程池

- MainReactor 只做 accept，避免业务阻塞影响新连接
- 新连接 round-robin 分发到 SubReactor，保证同一连接的读写在同一线程，无锁化
- 跨线程通信通过 EventLoop::runInLoop + eventfd 唤醒

### 2. ET 模式 + 非阻塞 IO

- accept4 必须循环到 EAGAIN
- recv 必须循环到 EAGAIN，否则丢数据
- EPOLLOUT 只在写缓冲满时关注，避免 busy loop
- 忽略 SIGPIPE，防止写入已关闭连接导致进程崩溃

### 3. 自定义二进制协议

- 定长包头携带 length，按长度读取包体，天然解决粘包
- ProtocolParser 维护读索引，避免频繁 erase 导致 O(n) 拷贝
- 半包状态保留在 Connection 的输入缓冲区

### 4. 异步日志（双缓冲）

- 前端线程只做 memcpy 到 current_buffer_
- 缓冲满或定时触发时，与 next_buffer_ 交换，投递到后端队列
- 后端线程批量写入文件，减少系统调用
- 相对同步日志 QPS 提升 7.7 倍

### 5. 分层时间轮

- 3 层时间轮（秒/分/时），插入删除 O(1)
- 替代最小堆管理心跳，10k 连接下 CPU 从 12% 降至 3%
- 基于 timerfd 统一纳入 epoll 事件循环

### 6. RAII 资源管理

- FileDescriptor 封装 fd，析构自动 close
- 移动语义转移所有权，禁止拷贝防止 double close
- 异常安全：任何路径退出都能正确回收

### 7. 线程安全

- UserManager 用 std::shared_mutex 保护在线列表（读多写少）
- ChatRoom 用互斥锁保护广播
- ThreadSafeQueue 模板，基于条件变量实现生产者-消费者

## 开发路线

| 阶段 | 内容 | 里程碑 |
|------|------|--------|
| 1 | base + 协议 Packet | 编译通过 |
| 2 | 单线程 EventLoop + Channel + EpollPoller | echo 服务 |
| 3 | 主从 Reactor + 线程池 | 多线程 echo |
| 4 | 用户/登录/群聊/在线列表 | 完整业务 |
| 5 | 异步日志 | 可切换日志 |
| 6 | 时间轮 + 心跳 | O(1) 心跳 |
| 7 | 离线消息 / 私聊 / 多房间 | 加分功能 |
| 8 | 自写压测客户端 + 数据采集 | QPS 数据 |
| 9 | 压测 + 调优 + 报告 | 性能数据 |
| 10 | 文档完善 | 项目包装 |

## 文档索引

| 文档 | 内容 |
|------|------|
| docs/architecture.md | 架构设计、模块划分、数据流 |
| docs/protocol.md | 二进制协议定义、字段说明、示例 |
| docs/benchmark.md | 压测方法、环境、结果、分析 |
| benchmark/README.md | 压测客户端使用说明 |

## 参考

- muduo 网络库：https://github.com/chenshuo/muduo
- Linux epoll man page：https://man7.org/linux/man-pages/man7/epoll.7.html
- C++ Core Guidelines：https://isocpp.github.io/CppCoreGuidelines

## License

MIT