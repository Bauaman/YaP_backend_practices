#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>


#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;


namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <base directory name>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        Logger logger;
        logger.InitLogging("game_server.log");
        /*
        logging::add_common_attributes();
        logging::add_file_log(keywords::file_name = "game_server.log",
                              keywords::format = "[%TimeStamp%]: %Message%",
                              keywords::open_mode = std::ios_base::app | std::ios_base::out);
        logging::add_console_log(std::clog,
                                 keywords::format = "[%TimeStamp%]: %Message%",
                                 keywords::auto_flush = true);
        */
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                //std::cout << "Signal "sv << signal_number << " received"sv << std::endl;
                ioc.stop();
                http_server::ReportServerExit(0);
            }
        });


        std::string root_dir = argv[2];
        fs::path root = fs::weakly_canonical(fs::path(root_dir));
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, root};

        http_handler::LoggingRequestHandler<http_handler::RequestHandler> loggingHandler{handler};

        //http_handler::LoggingRequestHandler loggingHandler{std::make_unique<http_handler::RequestHandler>(handler)};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        std::string root_dir = argv[2];
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        

        //BOOST_LOG_TRIVIAL(trace) << "Hello! Server is starting at port " << port << " (boostLog)"; 
        /*BOOST_LOG_TRIVIAL(info) 
            << logging::add_value(file, __FILE__) 
            << logging::add_value(line, __LINE__) 
            << "ServerStarted"sv; 
        */

        //std::cout << "Hello! Server is starting at port " << port << std::endl;
        /*
        http_server::ServeHttp(ioc, {address, port}, [&handler, root_dir](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), root_dir);
        }, root_dir);
        */

        boost::json::object add_data;
        add_data["port"] = port;
        add_data["address"] = address.to_string();
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server started";

        http_server::ServeHttp(ioc, {address, port}, [&loggingHandler, root_dir](auto&& req, auto&& send, const std::string& address_string) {
        loggingHandler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), root_dir, address_string);
        }, root_dir);

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        //std::cout << "Server has started..."sv << std::endl;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        //std::cerr << ex.what() << std::endl;
        http_server::ReportServerExit(EXIT_FAILURE, &ex);
        return EXIT_FAILURE;
    }
}
