// tcp_client.cpp
#include <iostream>
#include <string>
#include <thread>
#include <asio.hpp>

using namespace std;

class TcpClient {
public:
    TcpClient(const string& host, const string& port)
    : io_context_(), resolver_(io_context_), socket_(io_context_){

        cout << "Connecting to " << host << ":" << port << endl;

        auto endpoints = resolver_.resolve(host, port);
        asio::connect(socket_, endpoints);

        cout << "Connected!" << endl;
    }

void start(){
    std::thread receive_thread([this] {this->do_receive();});
    receive_thread.detach();

    do_send();
}

private:
    void do_receive(){
        try{
            while(true){
                char data[1024];
                asio::error_code error;
                size_t length = socket_.read_some(asio::buffer(data), error);
                
                if(error == asio::error::eof){
                    cout << "\nServer disconnected." << endl;
                    break; // 服务器关闭了连接
                }else if(error) {
                    throw asio::system_error(error);
                }

                cout.write(data, length);
                cout.flush();
            }
        } catch(std::exception& e){
            cerr << "Receive Error: " << e.what() << endl;
        }

        //接收线程结束
    }

    void do_send(){
        try{
            string line;
            while(getline(cin, line)){
                line += "\n";
                asio::write(socket_, asio::buffer(line));

                if(line == "quit\n"){
                    cout << "Disconnecting..." << endl;
                    break;
                }
            }
        } catch(std::exception& e){
            cerr << "Send Error: " << e.what() << endl;
        }
        // 关闭 socket，让接收线程也能感知到并退出
        socket_.close();
    }

    asio::io_context io_context_;
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
};

int main(int argc, char* argv[]){
    if(argc != 3){
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    try {
        TcpClient client(argv[1], argv[2]);
        client.start();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}