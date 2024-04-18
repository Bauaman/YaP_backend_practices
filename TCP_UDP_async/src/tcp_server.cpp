#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

using namespace boost::placeholders;

void handle_accept(socket_ptr sock, const boost::system::error_code& ec, tcp::acceptor& acc);
void async_read_message(socket_ptr sock);

void start_accept(tcp::acceptor& acc, socket_ptr sock) {
    std::cout << "Waiting for connection..." << std::endl;
    acc.async_accept(*sock, boost::bind(&handle_accept, sock, boost::asio::placeholders::error, boost::ref(acc)));
}

void handle_accept(socket_ptr sock, const boost::system::error_code& ec, tcp::acceptor& acc) {
    if (!ec) {
        std::cout << "Connection accepted from " << sock->remote_endpoint().address().to_string() << ":" << sock->remote_endpoint().port() << std::endl;
        socket_ptr new_sock(new tcp::socket(sock->get_executor()));
        async_read_message(sock);
    } else {
        std::cout << "Cannot accept connection error: " << ec.message() << std::endl;
        return;
    }
}

void on_write(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::cout << "Data sent successfully" << std::endl;
    } else {
        std::cerr << "Error during data sending: " << error.message() << std::endl;
    }
}

void on_read(std::shared_ptr<net::streambuf> buf, socket_ptr sock, const boost::system::error_code& ec, size_t bytes) {
    std::istream in(buf.get());
    std::string line;
    std::getline(in, line);
    std::cout << "Received message: " << line << std::endl;
    net::async_write(*sock, boost::asio::buffer("ok"), boost::bind(&on_write, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    //async_read_message(sock);
}

void async_read_message(socket_ptr sock) {
    //net::streambuf buf;
    std::shared_ptr<net::streambuf> buf = std::make_shared<net::streambuf>();
    net::async_read_until(*sock, *buf, '\n', boost::bind(&on_read, buf, sock, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

int main() {
    net::io_service service;
    tcp::endpoint ep(tcp::v4(), 3333);
    tcp::acceptor acc(service,ep);
    socket_ptr sock(new tcp::socket(service));
    start_accept(acc, sock);

    service.run();
}