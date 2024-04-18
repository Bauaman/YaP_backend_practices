#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;
using sock_ptr = boost::shared_ptr<tcp::socket>;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

using namespace std::literals;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponce(http::status status, std::string_view body, 
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status,http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& request) {
    const auto text_response = [&request](http::status status, std::string_view text) {
        return MakeStringResponce(status, text, request.version(), request.keep_alive());
    };
    return text_response(http::status::ok, "<stron>Hello</strong>"sv);
}

std::optional<StringRequest> ReadRequest(sock_ptr socket, beast::flat_buffer& buffer) {
    beast::error_code error;
    StringRequest req;
    http::read(*socket, buffer, req, error);

    if (error == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (error) {
        throw std::runtime_error("Failed to read request: "s.append(error.message()));
    }
    return req;
}

void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    for (const auto& header : req) {
        std::cout << " "sv << header.name_string() << ": "sv << header.value() << std::endl; 
    }
}

template <typename RequestHandler>
void HandleConnection(sock_ptr socket, RequestHandler&& handler) {
    try {
        beast::flat_buffer buffer;
        while (auto request = ReadRequest(socket, buffer)) {
            DumpRequest(*request);
            StringResponse response = handler(*std::move(request));
            response.set(http::field::content_type, "text/html"sv);
            http::write(*socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code error;
    socket->shutdown(tcp::socket::shutdown_send, error);
}

int main() {
    net::io_context io_context;
    
    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    constexpr std::string_view response
        = "HTTP/1.1 200 OK\r\n"sv
          "Content-Type: text/plain\r\n\r\n"sv
          "Hello"sv;
    
    tcp::acceptor acc(io_context, {address, port});

    std::cout << "Waiting for socket connection..."sv << std::endl;
    while (true) {
        sock_ptr socket(new tcp::socket(io_context));
        acc.accept(*socket);
        std::thread t(
            [](sock_ptr socket) {
                HandleConnection(socket, HandleRequest);
            }, std::move(socket));
        t.detach();        
    }
    //std::cout << "Connection received"sv << std::endl;
    //net::write(*sock, net::buffer(response));
}