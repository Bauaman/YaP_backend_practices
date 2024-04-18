#include <iostream>
#include <string>
#include <string_view>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::tcp;

using namespace std::literals;

int main() {
    static const int port = 3333;

    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..." << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection" << std::endl;
        return 1;
    }

    net::streambuf stream_buf;
    net::read_until(socket, stream_buf, '\n', ec);
    std::string client_data{std::istreambuf_iterator<char>(&stream_buf),
                            std::istreambuf_iterator<char>()};

    if (ec) {
        std::cout << "Error reading data" << std::endl;
        return 1;
    }

    std::cout << "Client said: " << client_data << std::endl;

    socket.write_some(net::buffer("Hello, I'm server!\n"), ec);

    if (ec) {
        std::cout << "Error sending data" << std::endl;
        return 1;
    }
}