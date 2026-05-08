#include "../include/Http_server.hpp"

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

const std::unordered_map<int, std::string> HTTP_STATUS_MAP = {
    { 200, "OK" },
    { 404, "NOT FOUND"},
    { 400, "Bad Request"},
    { 500, "Internal Server Error"}
};

HttpResponse::HttpResponse() : m_status_code(200), m_status_text("OK") {}

void HttpResponse::setStatusCode(int code){
    m_status_code = code;
    auto it = HTTP_STATUS_MAP.find(code);
    if(it != HTTP_STATUS_MAP.end()){
        m_status_text = it->second;
    }else{
        m_status_text = "Unknown Status Code";
    }
}

void HttpResponse::setHeader(const std::string& key, const std::string& value){
    m_headers[key] = value;
}

void HttpResponse::setBody(const std::string& content){
    m_body.assign(content.begin(), content.end());
}

void HttpResponse::setBody(const char* data, size_t length){
    m_body.assign(data, data+length);
}

std::string HttpResponse::buildResponseString() const {
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1" << m_status_code << m_status_text << "\r\n";
    for(const auto& [key, value] : m_headers){
        response_stream << key << ": " << value << "\r\n";
    }
    response_stream << "\r\n";
    response_stream.write(m_body.data(), m_body.length());
    return response_stream.str();
}

bool HttpRequestParser::parse(const std::string raw_request){
    size_t header_end_pos = raw_request.find("\r\n\r\n");
    if(header_end_pos == std::string::npos) return false;
    std::string header_section = raw_request.substr(0, header_end_pos + 4);

    std::istringstream header_stream(header_section);
    std::string request_line;
    if (!std::getline(header_stream, request_line)) return false;
    
    std::istringstream line_stream(request_line);
    if(!(line_stream >> m_methon >> m_uri >> std::ws)) return false;

    std::string header_line;
    while(std::getline(header_stream, header_line)){
        size_t colon_pos = header_line.find(":");
        if(colon_pos != std::string::npos){
            std::string key, value;
            key = header_line.substr(0, colon_pos);
            value = header_line.substr(colon_pos + 1);

            auto trim = [](std::string& s){
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned ch){ return !std::isspace(ch); }).base(), s.end());  
            };
            trim(key);
            trim(value);
            m_headers[key] = value;
        }
    }
    return true;
}

Connection::Connection(int client_fd)
    : fd(client_fd), send_size(0), parser(std::make_unique<HttpRequestParser>()), response(std::make_unique<HttpResponse>()){
};

Connection::~Connection(){
    close();
}

bool Connection::readFromSocket(){  
    char buffer[4096];
    ssize_t bytes_read = ::recv(fd, buffer, sizeof(buffer), 0);
    
    if(bytes_read > 0){
        request_buffer.append(buffer, bytes_read);
        if(request_buffer.find("\r\n\r\n") != std::string::npos){
            if(!parser->parse(request_buffer)){
                response->setStatusCode(400);
                response->setBody("<h1>400 Bad Request</h1>");
            }else if(parser-> getMethon() != "GET"){
                response->setStatusCode(400);
                response->setBody("Only Accept Get");
            }else{
                *response = processFileRequest(*parser, DEFAULT_ROOT_DIR);
            }
            response_buffer= response->buildResponseString();
        }
        return true;
    }else if(bytes_read == 0){
        return false;
    }else{
        if(errno == ERANGE || errno == EWOULDBLOCK){
            return true;
        }
        std::cerr << "srcv error: " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
        return false;
    }
}

bool Connection::writeSocket(){
    if(send_size >= response_buffer.size()){
        return false;
    }

    size_t bytes_sent = ::send(fd, response_buffer.c_str(), response_buffer.length(), MSG_NOSIGNAL);
    if(bytes_sent > 0){
        send_size += bytes_sent;
        return true;
    }else if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno ==EWOULDBLOCK))){
        return true;
    }else{
        std::cerr << "send error: " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
        return false;
    }
}

void Connection::close(){
    if(fd != -1){
        ::close(fd);
        fd = -1;
    }

}

Server::Server(const int port, const std::string& root_dir) : m_port(port), m_root_dir(root_dir){
    m_server_fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (m_server_fd < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    int opt = 1;
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr{};
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    

    if(::bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        throw std::runtime_error("bind error");
        close(m_server_fd);
    }

    if(::listen(m_server_fd, 10) < 0){
        throw std::runtime_error("listen error");
        close(m_server_fd);
    }

    set_NonBlocking(m_server_fd);


    m_epoll_fd = ::epoll_create1(0);
    if (m_epoll_fd < 0) {
        throw std::runtime_error("Epoll create failed: " + std::string(strerror(errno)));
    }

    epoll_event ev{};
    ev.data.fd = m_server_fd;
    ev.events = EPOLLIN;

    if(::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &ev) < 0){
        throw std::runtime_error("epoll_ctl failed.");
    }

    
}

Server::~Server(){
    if(m_server_fd != -1) close(m_server_fd);
    if(m_epoll_fd != -1) close(m_epoll_fd);
}

void Server::set_NonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::start(){
    const int MAX_EVENT = 1000;
    epoll_event events[MAX_EVENT];

    std::cout << "HTTP Server with epoll listening on port " << m_port << "...\n";
    std::cout << "Serving files from: " << m_root_dir << "\n";

    while(true){
        int nfds = ::epoll_wait(m_epoll_fd, events, MAX_EVENT, -1);
        if(nfds < 0){
            if(errno == EINTR) continue;
            throw std::runtime_error("Epoll wait failed: " + std::string(strerror(errno)));
        }

        for(int i = 0; i < nfds; i++){
            if(events[i].data.fd == m_server_fd){
                acceptNewConnection();
            }else {
                Connection* conn = static_cast<Connection*>(events[i].data.ptr);
                handleEvent(conn, events[i].events);
            }
        }
    }
}

void Server::acceptNewConnection(){
    int new_client_fd = ::accept(m_server_fd, nullptr, nullptr);
    if(new_client_fd < 0){
        std::cerr << "Accept failed: " << strerror(errno) << "\n";
        return;
    }

    set_NonBlocking(new_client_fd);

    auto conn = std::make_unique<Connection>(new_client_fd);
    int fd = new_client_fd;
    m_connection[fd] = std::move(conn);

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = m_connection[fd].get();
    if(::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0){
        std::cerr << "EPOLL add client_fd failed: " << strerror(errno) << "\n";
        m_connection.erase(fd);
    }
}

void Server::handleEvent(Connection* conn, uint32_t events){
    if(events && EPOLLIN){
        if(!conn->readFromSocket()){
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
            m_connection.erase(conn->fd);
            return;
        }

        if(!conn->response_buffer.empty()){
            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET;
            ev.data.ptr = conn;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev);
        }
    }

    if(events & EPOLLOUT){
        if(!conn->writeSocket()){
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
            m_connection.erase(conn->fd);
        }
    }
}

HttpResponse processFileRequest(const HttpRequestParser& parser, const std::string& root_dir){
    HttpResponse response;

    std::string uri = parser.getUri();
    if(uri == "/") uri = "/index.html";
    std::string filepath = root_dir + uri;

    if(!fileExists(filepath)){
        response.setStatusCode(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<h1>404 Not Found</h1><p>The requested file was not found.</p>");
        return response;
    }

    std::ifstream file(filepath, std::ios::binary);
    if(!file.is_open()){
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

std::string getMimeType(const std::string& filepath){
    size_t pos = filepath.find_last_of(".");
    if(pos == std::string::npos) return "application/octet-stream";
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

bool fileExists(const std::string& filepath){
    std::ifstream file(filepath);
    return file.is_open();
}