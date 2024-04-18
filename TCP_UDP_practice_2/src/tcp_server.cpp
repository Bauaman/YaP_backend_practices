#include <iostream>
#include <string>
#include <string_view>
#include <boost/asio.hpp>

namespace net = boost::asio;

using net::ip::tcp;
using sock_ptr = boost::shared_ptr<tcp::socket>;
using namespace std::literals;

std::string SendResponce(sock_ptr sock);
void StartGame(sock_ptr sock, bool my_init);

std::string ReadMessage(sock_ptr sock) {
    boost::system::error_code error;
    net::streambuf stream_buf;
    net::read_until(*sock, stream_buf, '\n', error);
    std::string rec_data;
    if (error) {
        std::cout << "Error reading data:" << error.message() << std::endl;
        return "error";
    }
    std::istream is(&stream_buf);
    std::getline(is, rec_data);
    std::cout << "Received: " << rec_data << std::endl;
    //SendResponce(sock);
    return rec_data;
}

std::string SendResponce(sock_ptr sock) {
    std::string result;
    std::cout << "Enter shot result: ";
    std::getline(std::cin, result);
    boost::system::error_code error;
    sock->write_some(net::buffer(result+'\n'), error);
    if (error) {
        std::cout << "Error sending message: " << error.message() << std::endl;
    }
    return result;
}

void SendMessage(sock_ptr sock) {
    std::string input;
    std::cout << "Input field for shot: ";
    std::getline(std::cin, input);
    boost::system::error_code error;
    sock->write_some(net::buffer(input+'\n'), error);
    if (error) {
        std::cout << "Error sending message: " << error.message() << std::endl;
        return;
    }
}

void StartServer(short port) {
    net::io_context io_context;
    tcp::endpoint ep(tcp::v4(), port);
    tcp::acceptor acc(io_context, ep);
    std::cout << "Waiting for connection..." << std::endl;

    boost::system::error_code error;
    sock_ptr sock(new tcp::socket(io_context));
    acc.accept(*sock, error);

    if (error) {
        std::cout << "Cannot accept connection: " << error.message() << std::endl;
        return;
    } else {
        std::cout << "Connected client: " << sock->remote_endpoint().address().to_string() << ":" << 
            sock->remote_endpoint().port() << std::endl;
        //ReadMessage(std::move(sock));
        StartGame(sock, false);
    }
}

void StartClient(const std::string& ip_addr, short port) {
    net::io_context io_context;
    boost::system::error_code error;
    tcp::endpoint ep(net::ip::make_address(ip_addr, error), port);
    if (error) {
        std::cout << "Error establishing connection: " << error.message() << std::endl;
    }
    sock_ptr sock(new tcp::socket(io_context));
    sock->connect(ep, error);
    if (error) {
        std::cout << "Can't connect to server" << error.message() << std::endl;
        return;
    } else {
        std::cout << "Connected to server" << std::endl;
        //SendMessage(sock);
        //ReadMessage(sock);
        StartGame(sock, true);
    }
}

bool IsGameEnded() {
    return false;
}

void StartGame(sock_ptr sock, bool my_init) {
    while(!IsGameEnded()) {
        std::string res;
        if (my_init) {
            while (res != "miss") {
                SendMessage(sock);
                res = ReadMessage(sock);
                std::cout << "my_init: " << my_init << std::endl;
            }
            my_init = !my_init;

        } else {
            while (res != "miss") {
                ReadMessage(sock);
                res = SendResponce(sock);
                std::cout << "my_init: " << my_init << std::endl;
            }
            my_init = !my_init;
        }
        std::cout << "my_init: " << my_init << std::endl;
    }
}


int main(int argc, char** argv) {
    if (strcmp(argv[1], "server") == 0) {
        StartServer(3333);
    }
    if (strcmp(argv[1], "client") == 0) {
        StartClient(argv[2], 3333);
    }
}