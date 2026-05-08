#ifndef HTTP_SERVER_V2_HPP
#define HTTP_SERVER_V2_HPP

#include<string>
#include<unordered_map>
#include<vector>
#include<memory>

static constexpr const char* DEFAULT_ROOT_DIR = "../www";

extern const std::unordered_map<int, std::string> HTTP_STATUS_MAP;

class HttpResponse{
private:
    int m_status_code;
    std::string m_status_text;
    std::unordered_map<std::string, std::string>m_headers;
    std::string m_body;
public:
    HttpResponse();
    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
    void setBody(const char* data, size_t length);
    std::string buildResponseString() const;
};

class HttpRequestParser{
private:
    std::string m_methon;
    std::string m_uri;
    std::unordered_map<std::string, std::string>m_headers;
    std::string m_body;
public:
    bool parse(const std::string raw_request);
    const std::string& getMethon() const { return m_methon; }
    const std::string& getUri() const { return m_uri; }
    const std::unordered_map<std::string, std::string>& getHeaders() const { return m_headers; }
    const std::string getBody() const { return m_body; }
};

class Connection{
public:
    int fd;
    std::string request_buffer;
    std::string response_buffer;
    size_t send_size;
    std::unique_ptr<HttpRequestParser> parser;
    std::unique_ptr<HttpResponse> response;

    Connection(int client_fd);
    ~Connection();

    bool readFromSocket();
    bool writeSocket();
    void close();
};

class Server{
private:
    int m_server_fd;
    int m_epoll_fd;
    int m_port;
    std::string m_root_dir;
    std::unordered_map<int, std::unique_ptr<Connection>> m_connection;
    void set_NonBlocking(int fd);
    void acceptNewConnection();
    void handleEvent(Connection* conne, uint32_t events);
public:
    Server(const int port = 8081, const std::string& root_dir = DEFAULT_ROOT_DIR);
    ~Server();
    void start();
};

HttpResponse processFileRequest(const HttpRequestParser& parser, const std::string& root_dir);
std::string getMimeType(const std::string& file_extension);
bool fileExists(const std::string& filepath);

#endif