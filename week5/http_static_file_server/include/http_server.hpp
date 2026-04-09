#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <unordered_map> // 用于管理活跃连接

// 定义一个常量，指向服务器默认查找文件的根目录
static constexpr const char* DEFAULT_ROOT_DIR = "../www";

// 全局常量，用于将 HTTP 状态码映射到其描述文字
extern const std::unordered_map<int, std::string> HTTP_STATUS_MAP;

// --- 类的声明 ---

// 1. HttpResponse 类：用于构建服务器的响应
class HttpResponse {
private:
    int m_status_code;
    std::string m_status_text;
    std::unordered_map<std::string, std::string> m_headers;
    std::vector<char> m_body;

public:
    HttpResponse();
    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
    void setBody(const char* data, size_t length);
    std::string buildResponseString() const;
};

// 2. HttpRequestParser 类：用于解析客户端发来的请求
class HttpRequestParser {
private:
    std::string m_method;
    std::string m_uri;
    std::unordered_map<std::string, std::string> m_headers;

public:
    bool parse(const std::string& raw_request);
    const std::string& getMethod() const { return m_method; }
    const std::string& getUri() const { return m_uri; }
    const std::unordered_map<std::string, std::string>& getHeaders() const { return m_headers; }
};

// 3. Connection 类：代表一个活跃的客户端连接
class Connection {
public:
    int fd; // 客户端的文件描述符
    std::string request_buffer; // 存储从客户端接收到的数据
    std::string response_buffer; // 存储准备发送给客户端的响应数据
    size_t sent_bytes; // 已发送的字节数
    std::unique_ptr<HttpRequestParser> parser; // 解析器
    std::unique_ptr<HttpResponse> response; // 响应对象

    Connection(int client_fd);
    ~Connection();

    bool readFromSocket();
    bool writeToSocket();
    void close();
};

// 4. Server 类：核心服务器类，封装了 epoll 和所有逻辑
class Server {
private:
    int m_server_fd; // 监听的 socket 文件描述符
    int m_epoll_fd; // epoll 实例的文件描述符
    int m_port; // 服务器端口
    std::string m_root_dir; // 静态文件根目录
    std::unordered_map<int, std::unique_ptr<Connection>> m_connections; // 存储所有活跃连接

    void setNonBlocking(int fd);
    void acceptNewConnection();
    void handleEvent(Connection* conn, uint32_t events);

public:
    explicit Server(int port = 8080, const std::string& root_dir = DEFAULT_ROOT_DIR);
    ~Server();

    void start(); // 启动服务器主循环
};

// --- 独立函数的声明 ---
HttpResponse processFileRequest(const HttpRequestParser& parser, const std::string& root_dir);
std::string getMimeType(const std::string& file_extension);
bool fileExists(const std::string& filepath);

#endif // HTTP_SERVER_HPP