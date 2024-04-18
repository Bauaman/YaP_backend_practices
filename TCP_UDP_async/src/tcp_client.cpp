#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

//void on_write(const boost::system::error_code& error, size_t bytes_transferred);
//void write_message(socket_ptr sock);

void on_read(std::shared_ptr<net::streambuf> buf, socket_ptr sock, const boost::system::error_code& ec, size_t bytes) {
    std::istream in(buf.get());
    std::string line;
    std::getline(in, line);
    std::cout << "Received message: " << line << std::endl;
    //net::async_write(*sock, boost::asio::buffer("ok"), boost::bind(&on_write, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    //async_read_message(sock);
}

void async_read_message(socket_ptr sock) {
    //net::streambuf buf;
    std::shared_ptr<net::streambuf> buf = std::make_shared<net::streambuf>();
    net::async_read_until(*sock, *buf, '\n', boost::bind(&on_read, buf, sock, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void connect_handler(const boost::system::error_code& ec, socket_ptr sock) {
    if (!ec) {
        std::cout << "Connected" << std::endl;
        std::string message = "Hello, server!";
        net::async_write(*sock, net::buffer(message), [](const boost::system::error_code& error, size_t bytes_transferred) {
            if (!error) {
                std::cout << "Message sent successfully" << std::endl;

            } else {
                std::cout << "Error sending message" << std::endl;
                return;
            }
        });
        async_read_message(sock);
    } else {
        std::cout << "Connection error: " << ec.message() << std::endl;
    }
}
/*
void write_message(socket_ptr sock) {
    std::string input;
    std::getline(std::cin, input);
    if (input == "exit") {
        return;
    }
    net::async_write(*sock, boost::asio::buffer(input), boost::bind(&on_write, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    net::async_read_until(*sock, boost::asio::dynamic_buffer(input), '\n', [&](const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error) {
            std::cout << "Received: " << input << std::endl;
            write_message(sock); // Рекурсивный вызов для чтения и отправки следующего сообщения
        } else {
            std::cerr << "Error during data receiving: " << error.message() << std::endl;
        }
    });
}

void on_write(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::cout << "Data sent successfully" << std::endl;
    } else {
        std::cerr << "Error during data sending: " << error.message() << std::endl;
    }
}
*/
int main() {
    net::io_service service;
    tcp::endpoint ep(net::ip::address::from_string("127.0.0.1"), 3333);
    socket_ptr sock(new tcp::socket(service));
    sock->async_connect(ep, boost::bind(&connect_handler, boost::asio::placeholders::error, sock));
    service.run();

}