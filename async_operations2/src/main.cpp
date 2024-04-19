#include <boost/asio.hpp>
#include <chrono>
#include <iomanip>
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
/*
//  Функция POST
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
    
    //std::jthread{[&io_context] {
    //    io_context.run();
    //}}.join();
    
    std::cout << "Sleep" << std::endl;
}*/

/*
//  Функция DEFER

int main() {
    using namespace std::chrono;

    //net::thread_pool tp{2};
    net::io_context io{2};
    
    const auto start = steady_clock::now();

    auto print = [start](char ch) {
        const auto t = duration_cast<duration<double>>(steady_clock::now() - start).count();
        std::osyncstream(std::cout) << std::fixed << std::setprecision(6) << t << "> " << ch
                                    << ':' << std::this_thread::get_id() << std::endl; 
    };

    net::post(io, [&io, print] {
        print('A');

        net::defer(io, [print]{print('B'); });
        net::defer(io, [print]{print('C'); });
        net::defer(io, [print]{print('D'); });

        std::this_thread::sleep_for(std::chrono::seconds(1));
    });

    //tp.wait();
    RunWorkers(2, [&io]{io.run(); });
}*/

/*
//Функция DISPATCH
int main() {
    net::io_context io;

    // dispatch поместит вызов лямбда-функции в очередь, так как dispatch вызван вне io.run()
    net::dispatch(io, [&io] {
        std::cout << "Work hard" << std::endl;

        // post всегда выполняет функцию асинхронно
        net::post(io, [] {
            std::cout << "Have dinner" << std::endl;
        });

        // Этот dispatch вызовет лямбда функцию синхронно, так как dispatch вызван внутри io.run
        net::dispatch(io, [] {
            std::cout << "Eat an apple" << std::endl;
        });

        std::cout << "Go home" << std::endl;
    });

    std::cout << "Go to work" << std::endl;
    io.run();
    std::cout << "Sleep" << std::endl;
}*/
/*
int main() {
    net::io_context io;

    net::post(io, [&io] {  // (1)
        std::cout << 'A';
        net::post(io, [] {  // (2)
            std::cout << 'B';
        });
        std::cout << 'C';
    });

    net::dispatch(io, [&io] {  // (3)
        std::cout << 'D';
        net::post(io, [] {  // (4)
            std::cout << 'E';
        });
        net::defer(io, [] {  // (5)
            std::cout << 'F';
        });
        net::dispatch(io, [] {  // (6)
            std::cout << 'G';
        });
        std::cout << 'H';
    });

    std::cout << 'I';
    io.run();
    std::cout << 'J';
}*/

int main() {
    using osync = std::osyncstream;

    net::io_context io;
    auto strand1 = net::make_strand(io);

    net::post(strand1, [strand1] {  // (1)
        net::post(strand1, [] {     // (2)
            osync(std::cout) << 'A';
        });
        net::dispatch(strand1, [] {  // (3)
            osync(std::cout) << 'B';
        });
        osync(std::cout) << 'C';
    });

    auto strand2 = net::make_strand(io);
    // Эти функции выполняются в strand2
    net::post(strand2, [strand2] {  // (4)
        net::post(strand2, [] {     // (5)
            osync(std::cout) << 'D';
        });
        net::dispatch(strand2, [] {  // (6)
            osync(std::cout) << 'E';
        });
        osync(std::cout) << 'F';
    });

    RunWorkers(2, [&io] {
        io.run();
    });
}