#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>
#include <thread>

//#include "aux.h"
//#include "logger.h"
//#include "game_server.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace {

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

} // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <base directory name>"sv << std::endl;
        return EXIT_FAILURE;
    }
    logger::Logger logger;
    logger.Init();

    try {
        fs::path config = fs::weakly_canonical(fs::path(auxillary::UrlDecode(argv[1])));
        fs::path root = fs::weakly_canonical(fs::path(auxillary::UrlDecode(argv[2])));
        
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        GameServer gs(ioc, config, root);

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &logger](const sys::error_code ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                //logger::LogExit(0);
            }
        });

        auto handler = std::make_shared<http_handler::RequestHandler>(ioc, gs);
        http_handler::LoggingRequestHandler<http_handler::RequestHandler> logging_handler{*handler};
        boost::json::object add_data;
        add_data["port"] = port;
        add_data["address"] = address.to_string();
        logger::LogMessageInfo(add_data, "server started"s);
    // Запускаем обработку запросов 
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        logger::LogExit(EXIT_FAILURE, &ex);
        return EXIT_FAILURE;
    }
    logger::LogExit(0);
    return 0;

}