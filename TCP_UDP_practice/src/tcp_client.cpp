#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;

using namespace std::literals;

int main(int argc, char** argv) {
    static const int port = 3333;
    boost::system::error_code ec;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <server IP>" << std::endl;
        return 1;
    }

    // Создадим endpoint - объект с информацией об адресе и порте.
    // Для разбора IP-адреса пользуемся функцией net::ip::make_address.

    auto endpoint = tcp::endpoint(net::ip::make_address(argv[1], ec), port);

    if (ec) {
        std::cout << "Wrong IP format" << std::endl;
        return 1;
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server" << std::endl;
        return 1;
    }

    // Отправляем данные и проверяем, что нет ошибки.
    socket.write_some(net::buffer("Hello, I'm client!\n"), ec);
    if (ec) {
        std::cout << "Error sending data" << std::endl;
        return 1;
    }

    net::streambuf stream_buf;
    net::read_until(socket, stream_buf, '\n', ec);
    std::string server_data{std::istreambuf_iterator<char>(&stream_buf),
                            std::istreambuf_iterator<char>()};

    if (ec) {
        std::cout << "Error reading data" << std::endl;
        return 1;
    }

    std::cout << "Server responded: " << server_data << std::endl;
}