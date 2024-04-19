#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <syncstream>
#include <thread>

namespace net = boost::asio;
using namespace std::chrono_literals;

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

int main() {
    net::io_context io_context;
    std::cout << "Eat. Thread id: " << std::this_thread::get_id() << std::endl;

    net::post(io_context, [] {
        std::cout << "Wash dishes. Thread id: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(1s);
    });

    net::post(io_context, [] {
        std::cout << "Clean table. Thread id: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(1s);
    });

    net::post(io_context, [&io_context] {  // Пылесосим комнату
        std::osyncstream(std::cout) << "Vacuum-clean room. Thread id: " << std::this_thread::get_id()
                         << std::endl;
        std::this_thread::sleep_for(1s);

        net::post(io_context, [] {  // После того, как пропылесосили, асинхронно моем пол
            std::osyncstream(std::cout) << "Wash floor. Thread id: " << std::this_thread::get_id()
                             << std::endl;
            std::this_thread::sleep_for(1s);
        });

        net::post(io_context, [] {  // Асинхронно опустошаем пылесборник пылесоса
            std::osyncstream(std::cout) << "Empty vacuum cleaner. Thread id: " << std::this_thread::get_id()
                             << std::endl;
            std::this_thread::sleep_for(1s);
        });
    });

    std::cout << "Work. Thread id:" << std::this_thread::get_id() << std::endl;
    
    RunWorkers(2, [&io_context]{
        io_context.run();
    });
    /*
    std::jthread{[&io_context] {
        io_context.run();
    }}.join();
    */
    std::cout << "Sleep" << std::endl;



}