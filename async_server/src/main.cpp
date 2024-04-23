#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
#include <thread>
#include <vector>
#include <type_traits>

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
//namespace http = boost::beast::http;
//using StringResponse = http::response<http::string_body>;

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n-1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

namespace http_server {

    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace sys = boost::system;
    namespace http = beast::http;
    using namespace std::literals;
    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;

    void ReportError(beast::error_code ec, std::string_view what) {
        std::cerr << what << ": "sv << ec.message() << std::endl;
    }

    class SessionBase {
    public:
        SessionBase(const SessionBase&) = delete;
        SessionBase& operator=(const SessionBase&) = delete;

        void Run();

    protected:
        explicit SessionBase(tcp::socket&& socket) :
            stream_(std::move(socket)) {
        }

        using HttpRequest = http::request<http::string_body>;

        template <typename Body, typename Fields>
        void Write(http::response<Body, Fields>&& response) {
            auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
            auto self = GetSharedThis();
            http::async_write(stream_, *safe_response, 
                              [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                              });
        }

        ~SessionBase() = default;

    private:
        void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
            if (ec) {
                return ReportError(ec, "write"sv);
            }
            if (close) {
                // Семантика ответа требует закрыть соединение
                return Close();
            }
            // Считываем следующий запрос
            Read();
        }

        void Read() {
            using namespace std::literals;
            // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
            request_ = {};
            stream_.expires_after(30s);
            // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
            http::async_read(stream_, buffer_, request_,
                            // По окончании операции будет вызван метод OnRead
                            beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
        }

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        using namespace std::literals;
        if (ec == http::error::end_of_stream) {
            // Нормальная ситуация - клиент закрыл соединение
            return Close();
        }
        if (ec) {
            return ReportError(ec, "read"sv);
        }
        HandleRequest(std::move(request_));
    }

    void Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    // Обработку запроса делегируем подклассу
    virtual void HandleRequest(HttpRequest&& request) = 0;

        virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        HttpRequest request_;
    };

    template <typename RequestHandler>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
    public:
        template <typename Handler>
        Session(tcp::socket&& socket, Handler&& request_handler) :
            SessionBase(std::move(socket)),
            request_handler_(std::forward<Handler>(request_handler)) {
        }

    private:
        void HandleRequest(HttpRequest&& request) override {
            request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
            });
        }

        std::shared_ptr<SessionBase> GetSharedThis() override {
            return this->shared_from_this();
        }  
        RequestHandler request_handler_;
    };

    template <typename RequestHandler>
    void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request) {
        try {
            beast::flat_buffer buffer;
            while (auto request = ReadRequest(socket, buffer)) {
                StringResponse response = handle_request(*std::move(request));
                http::write(socket, response);
                if (response.need_eof()) {
                    break;
                }
            }            
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        beast::error_code ec;

        socket.shutdown(tcp::socket::shutdown_send, ec);
    }

    template <typename RequestHandler>
    class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
    public:
        template <typename Handler>
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler_) :
            ioc_{ioc},
            acceptor_{net::make_strand(ioc)},
            request_handler_(std::forward<Handler>(request_handler_)) {
                acceptor_.open(endpoint.protocol());
                acceptor_.set_option(net::socket_base::reuse_address(true));
                acceptor_.bind(endpoint);
                acceptor_.listen(net::socket_base::max_listen_connections);
            }

        void Run() {
            DoAccept();
        }

    private:
        void DoAccept() {
            acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
        }

        void OcAccept(sys::error_code ec, tcp::socket socket) {
            using namespace std::literals;
            if(ec) {
                return ReportError(ec, "accept"sv);
            }
            AsyncRunSession(std::move(socket));
            DoAccept();
        }

        void AsyncRunSession() {

        }
        void AsyncRunSession(tcp::socket&& socket) {
            std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
        }

        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestHandler request_handler_;
    };

    void SessionBase::Run() {
        // Вызываем метод Read, используя executor объекта stream_.
        // Таким образом вся работа со stream_ будет выполняться, используя его executor
        net::dispatch(stream_.get_executor(),
                    beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    template <typename RequestHandler>
    void ServerHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
        using MyListener = Listener<std::decay_t<RequestHandler>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
    }

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

    StringResponse HandleRequest(StringRequest&& req) {
        http::status status;
        std::string text;
        if ((req.method() != http::verb::get) && (req.method() != http::verb::head)) {
            status = http::status::method_not_allowed;
            text = "Invalid method";
        } else {
            status = http::status::ok;
            text = "Hello, ";
            text.append(req.target().substr(1));
        }
        const auto text_response = [&req](http::status status, std::string_view text) {
            return MakeStringResponce(status, text, req.version(), req.keep_alive());
        };
        return text_response(status, text);
    }

} //namespace http_server



int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            std::cout << "Signal "sv << signal_number << " received"sv << std::endl;
            ioc.stop();
        }
    });

    net::steady_timer t{ioc, 30s};
    t.async_wait([](sys::error_code ec) {
        std::cout << "Timer expired"s << std::endl;
    });

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
    std::cout << "Shutting down"sv << std::endl;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServerHttp(ioc, {address, port}, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
    });

    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
    });
}