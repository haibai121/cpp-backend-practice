#include "../include/http_server.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h> // 用于 fcntl 设置非阻塞

// 定义全局常量 HTTP_STATUS_MAP
const std::unordered_map<int, std::string> HTTP_STATUS_MAP = {
    {200, "OK"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {500, "Internal Server Error"}
};

// --- HttpResponse 实现 ---
HttpResponse::HttpResponse() : m_status_code(200), m_status_text("OK") {}

void HttpResponse::setStatusCode(int code) {
    m_status_code = code;
    auto it = HTTP_STATUS_MAP.find(code);
    if (it != HTTP_STATUS_MAP.end()) {
        m_status_text = it->second;
    } else {
        m_status_text = "Unknown Status Code";
    }
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    m_headers[key] = value;
}

void HttpResponse::setBody(const std::string& content) {
    m_body.assign(content.begin(), content.end());
}

void HttpResponse::setBody(const char* data, size_t length) {
    m_body.assign(data, data + length);
}

std::string HttpResponse::buildResponseString() const {
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 " << m_status_code << " " << m_status_text << "\r\n";
    for (const auto& [key, value] : m_headers) {
        response_stream << key << ": " << value << "\r\n";
    }
    response_stream << "\r\n";
    response_stream.write(m_body.data(), m_body.size());
    return response_stream.str();
}

// --- HttpRequestParser 实现 ---
bool HttpRequestParser::parse(const std::string& raw_request) {
    //这三行代码是为了提取出除请求体之外的部分，以便专心处理请求头和请求行
    size_t header_end_pos = raw_request.find("\r\n\r\n");
    if (header_end_pos == std::string::npos) return false;
    //这里substr的意思是把刚提取到的\r\n\r\n之前的数据数据和\r\n\r\n一起保存到headers_section里面进行存储，以便专心处理请求头和请求行
    //所以substr就是把从raw_request里面从0开始到找到\r\n\r\n部分的长度(header_end_pos + 4)全部存储到headers_section中
    std::string headers_section = raw_request.substr(0, header_end_pos + 4);

    std::istringstream header_stream(headers_section);
    std::string request_line;
    if (!std::getline(header_stream, request_line)) return false;

    std::istringstream line_stream(request_line);
    if (!(line_stream >> m_method >> m_uri >> std::ws)) {
        return false;
    }

    std::string header_line;
    while (std::getline(header_stream, header_line) && header_line != "\r") {
        auto colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);//从0开始，到colon_pos
            std::string value = header_line.substr(colon_pos + 1);//从colon_pos+1开始到结束，因为没有指定长度count，所以默认为到末尾

            //s.erase(pos, len) 或 s.erase(first, last) 的作用是从字符串 s 中删除指定位置或范围的字符
            //这里的作用就是删除空格和\r
            auto trim = [](std::string& s) {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            };
            trim(key); trim(value);
            m_headers[key] = value;
        }
    }
    return true;
}

// --- Connection 类实现 ---
Connection::Connection(int client_fd) 
    : fd(client_fd), sent_bytes(0), parser(std::make_unique<HttpRequestParser>()), response(std::make_unique<HttpResponse>()) {
}

Connection::~Connection() {
    close();
}

bool Connection::readFromSocket() {
    char buffer[1024];
    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read > 0) {
        request_buffer.append(buffer, bytes_read);
        if (request_buffer.find("\r\n\r\n") != std::string::npos) {
            if (!parser->parse(request_buffer)) {
                response->setStatusCode(400);
                response->setBody("<h1>400 Bad Request</h1>");
            } else if (parser->getMethod() != "GET") {
                response->setStatusCode(400);
                response->setBody("<h1>400 Bad Request - Only GET allowed</h1>");
            } else {
                *response = processFileRequest(*parser, DEFAULT_ROOT_DIR);
            }
            response_buffer = response->buildResponseString();
        }
        return true;
    } else if (bytes_read == 0) {
        return false; // 客户端关闭了连接
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true; // 非阻塞，暂时无数据
        }
        return false; // 真正的错误
    }
}

bool Connection::writeToSocket() {
    if (sent_bytes >= response_buffer.size()) {
        return false; // 数据已全部发送完毕
    }

    ssize_t bytes_sent = send(fd, response_buffer.c_str() + sent_bytes, response_buffer.size() - sent_bytes, MSG_NOSIGNAL);
    if (bytes_sent > 0) {
        sent_bytes += bytes_sent;
        return true; // 继续写
    } else if (bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        return true; // 发送缓冲区满，稍后再来
    } else {
        return false; // 真正的错误
    }
}

void Connection::close() {
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

// --- Server 类实现 ---
Server::Server(int port, const std::string& root_dir) : m_port(port), m_root_dir(root_dir) {
    // 1. 创建监听 socket
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    // 2. 设置 SO_REUSEADDR
    int opt = 1;
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定和监听
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    if (bind(m_server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }

    if (listen(m_server_fd, 10) < 0) {
        throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
    }

    // 4. 设置监听 socket 为非阻塞
    setNonBlocking(m_server_fd);

    // 5. 创建 epoll 实例
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd < 0) {
        throw std::runtime_error("Epoll create failed: " + std::string(strerror(errno)));
    }

    // 6. 将监听 socket 加入 epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = m_server_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &ev) < 0) {
        throw std::runtime_error("Epoll add server_fd failed: " + std::string(strerror(errno)));
    }
}

Server::~Server() {
    if (m_epoll_fd != -1) close(m_epoll_fd);
    if (m_server_fd != -1) close(m_server_fd);
}

void Server::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::start() {
    const int MAX_EVENTS = 1000;
    epoll_event events[MAX_EVENTS];

    std::cout << "HTTP Server with epoll listening on port " << m_port << "...\n";
    std::cout << "Serving files from: " << m_root_dir << "\n";

    while (true) {
        int nfds = epoll_wait(m_epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue; // 信号中断，继续
            throw std::runtime_error("Epoll wait failed: " + std::string(strerror(errno)));
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == m_server_fd) {
                acceptNewConnection();
            } else {
                Connection* conn = static_cast<Connection*>(events[i].data.ptr);
                handleEvent(conn, events[i].events);
            }
        }
    }
}

void Server::acceptNewConnection() {
    int new_client_fd = accept(m_server_fd, nullptr, nullptr);
    if (new_client_fd < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << "\n";
        return;
    }

    setNonBlocking(new_client_fd);

    // 创建并管理新的连接
    auto conn = std::make_unique<Connection>(new_client_fd);
    int fd = new_client_fd; // 保存 fd 用于 map 的 key
    m_connections[fd] = std::move(conn);

    // 将新连接加入 epoll 监控
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = m_connections[fd].get(); // 将 Connection 对象指针存入事件数据
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "Epoll add client_fd failed: " << strerror(errno) << "\n";
        m_connections.erase(fd); // 添加失败，从 map 中移除
    }
}

void Server::handleEvent(Connection* conn, uint32_t events) {
    if (events & EPOLLIN) {
        if (!conn->readFromSocket()) {
            // 读取失败，关闭连接
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
            m_connections.erase(conn->fd);
            return;
        }
        
        // 如果响应已准备好，修改为监听写事件
        if (!conn->response_buffer.empty()) {
            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET;
            ev.data.ptr = conn;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev);
        }
    }

    if (events & EPOLLOUT) {
        if (!conn->writeToSocket()) {
            // 写入失败或完成，关闭连接
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
            m_connections.erase(conn->fd);
        }
    }
}

// --- 独立函数实现 ---
HttpResponse processFileRequest(const HttpRequestParser& parser, const std::string& root_dir) {
    HttpResponse response;

    std::string uri = parser.getUri();
    if (uri == "/") uri = "/index.html";
    std::string filepath = root_dir + uri;

    if (!fileExists(filepath)) {
        response.setStatusCode(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<h1>404 Not Found</h1><p>The requested file was not found.</p>");
        return response;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        response.setStatusCode(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<h1>500 Internal Server Error</h1><p>Could not read the file.</p>");
        return response;
    }

    std::vector<char> file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    response.setStatusCode(200);
    response.setHeader("Content-Type", getMimeType(filepath));
    response.setHeader("Content-Length", std::to_string(file_content.size()));
    response.setBody(file_content.data(), file_content.size());

    return response;
}

std::string getMimeType(const std::string& filepath) {
    size_t pos = filepath.find_last_of('.');
    if (pos == std::string::npos) return "application/octet-stream";
    std::string ext = filepath.substr(pos + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "txt") return "text/plain";
    return "application/octet-stream";
}

bool fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.is_open();
}