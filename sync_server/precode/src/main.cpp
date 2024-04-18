#ifdef WIN32
#include <sdkddkver.h>
#endif
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;
using namespace std::literals;
using sock_ptr = boost::shared_ptr<tcp::socket>;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;


struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

StringResponse MakeStringResponce(http::status status, std::string_view body, 
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML) {

    //std::cout << "MakeStringResponce: status " << status << std::endl;
    //std::cout << "MakeStringResponce: body " << body << std::endl;
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    if (status == http::status::method_not_allowed) {
        response.set(http::field::allow, "GET, HEAD"sv);
    }
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& request) {
    http::status status;
    std::string text;
    if ((request.method() != http::verb::get) && (request.method() != http::verb::head)) {
        status = http::status::method_not_allowed;
        text = "Invalid method";
    } else {
        status = http::status::ok;
        text = "Hello, ";
        text.append(request.target().substr(1));
    }


    const auto text_response = [&request](http::status status, std::string_view text) {
        return MakeStringResponce(status, text, request.version(), request.keep_alive());
    };
    return text_response(status, text);
}

void DumpRequest(const StringRequest& req) {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    for (const auto& header : req) {
        std::cout << " "sv << header.name_string() << ": "sv << header.value() << std::endl; 
    }
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

template <typename RequestHandler>
void HandleConnection (sock_ptr socket, RequestHandler handler) {
    try {
        beast::flat_buffer buffer;
        while (auto request = ReadRequest(socket, buffer)) {
            //DumpRequest(*request);
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
    tcp::acceptor acc(io_context, {address, port});
    std::cout << "Server has started..."sv << std::endl;
    while (true) {
        sock_ptr socket(new tcp::socket(io_context));
        acc.accept(*socket);
        std::thread t(
            [](sock_ptr socket) {HandleConnection(socket, HandleRequest);}, std::move(socket));
        t.detach();
    }
}
