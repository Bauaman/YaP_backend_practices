#include <deque>
#include <iostream>
#include <latch>
#include <syncstream>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <variant>
#include <type_traits>
#include <stdexcept>
#include <optional>

namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::literals;
using Clock = std::chrono::high_resolution_clock;
using namespace std::chrono;

template <typename ValueType>
class Result {
public:
    Result(const ValueType& value) noexcept(std::is_nothrow_constructible_v<ValueType>) :
        state_{value} {
    }

    Result(const ValueType&& value) noexcept(std::is_nothrow_constructible_v<ValueType>) :
        state_{std::move(value)} {
    }

    Result(std::nullptr_t) = delete;

    bool HasValue() const noexcept {
        return std::holds_alternative<ValueType>(state_);
    }

    const ValueType& GetValue() const& {
        return std::get<ValueType>(state_);
    }

    ValueType&& GetValue() && {
        return std::get<ValueType>(std::move(state_));
    }

private:
    std::variant<ValueType, std::exception_ptr> state_;
};

class GasCooker : public std::enable_shared_from_this<GasCooker> {
public:
    using Handler = std::function<void()>;

    GasCooker(net::io_context& io, int num_burners = 1)
        : io_{io}
        , number_of_burners_{num_burners} {
    }

    GasCooker(const GasCooker&) = delete;
    GasCooker& operator=(const GasCooker&) = delete;

    ~GasCooker() {
        assert(burners_in_use_ == 0);
    }

    // Используется для того, чтобы занять горелку. handler будет вызван в момент, когда горелка
    // занята
    // Этот метод можно вызывать параллельно с вызовом других методов
    void UseBurner(int sid, std::chrono::_V2::steady_clock::time_point start_time, Handler handler) {
        // Выполняем работу внутри strand, чтобы изменение состояния горелки выполнялось
        // последовательно
        net::dispatch(strand_,
                      // За счёт захвата self в лямбда-функции, время жизни GasCooker будет продлено
                      // до её вызова
                      [sid, start_time, handler = std::move(handler), self = shared_from_this(), this]() mutable {
                          assert(strand_.running_in_this_thread());
                          assert(burners_in_use_ >= 0 && burners_in_use_ <= number_of_burners_);

                          // Есть свободные горелки?
                          if (burners_in_use_ < number_of_burners_) {
                              // Занимаем горелку
                              ++burners_in_use_;
                              // Асинхронно уведомляем обработчик о том, что горелка занята.
                              // Используется асинхронный вызов, так как handler может
                              // выполняться долго, а strand лучше освободить
                              std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Burners in use " << burners_in_use_ << std::endl;
                              net::post(io_, std::move(handler));
                          } else {  // Все горелки заняты
                              // Ставим обработчик в хвост очереди
                              pending_handlers_.emplace_back(std::move(handler));
                              std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Sausage #" << sid << " moved to pending" << std::endl;
                              std::osyncstream(std::cout) << "Handlers pending: " << pending_handlers_.size() << std::endl;
                          }

                          // Проверка инвариантов класса
                          assert(burners_in_use_ > 0 && burners_in_use_ <= number_of_burners_);
                      });
    }

    void ReleaseBurner() {
        // Освобождение выполняем также последовательно
        net::dispatch(strand_, [this, self = shared_from_this()] {
            assert(strand_.running_in_this_thread());
            assert(burners_in_use_ > 0 && burners_in_use_ <= number_of_burners_);

            // Есть ли ожидающие обработчики?
            if (!pending_handlers_.empty()) {
                // Выполняем асинхронно обработчик первый обработчик
                net::post(io_, std::move(pending_handlers_.front()));
                // И удаляем его из очереди ожидания
                pending_handlers_.pop_front();
            } else {
                // Освобождаем горелку
                --burners_in_use_;
            }
        });
    }

private:
    using Strand = net::strand<net::io_context::executor_type>;
    net::io_context& io_;
    Strand strand_{net::make_strand(io_)};
    int number_of_burners_;
    int burners_in_use_ = 0;
    std::deque<Handler> pending_handlers_;
};

// RAII-класс для автоматического освобождения газовой плиты
class GasCookerLock {
public:
    GasCookerLock() = default;

    explicit GasCookerLock(std::shared_ptr<GasCooker> cooker) noexcept
        : cooker_{std::move(cooker)} {
    }

    GasCookerLock(GasCookerLock&& other) = default;
    GasCookerLock& operator=(GasCookerLock&& rhs) = default;

    GasCookerLock(const GasCookerLock&) = delete;
    GasCookerLock& operator=(const GasCookerLock&) = delete;

    ~GasCookerLock() {
        try {
            Unlock();
        } catch (...) {
        }
    }

    void Unlock() {
        if (cooker_) {
            cooker_->ReleaseBurner();
            cooker_.reset();
        }
    }

private:
    std::shared_ptr<GasCooker> cooker_;
};

class Sausage : public std::enable_shared_from_this<Sausage> {
public:
    using Handler = std::function<void()>;
    explicit Sausage(int id, net::io_context& io) :
        id_{id}, 
        io_{io} {
    }

    int GetId() const {
        return id_;
    }

    void StartFry(GasCooker& cooker, std::chrono::_V2::steady_clock::time_point start_time, Handler handler) {
        if (frying_start_time_) {
            throw std::logic_error("Frying already started");
        }

        // Запрещаем повторный вызов StartFry
        frying_start_time_ = Clock::now();

        // Готовимся занять газовую плиту
        gas_cooker_lock_ = GasCookerLock{cooker.shared_from_this()};

        // Занимаем горелку для начала обжаривания.
        // Чтобы продлить жизнь текущего объекта, захватываем shared_ptr в лямбде
        cooker.UseBurner(id_, start_time, [id = id_, start_time, fry_timer_ = &fry_timer, strand = std::move(strand_), self = shared_from_this(), handler = std::move(handler)](){
            // Запоминаем время фактического начала обжаривания
            self->frying_start_time_ = Clock::now();
            //handler();
            std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Sausage #" << id << " put on burner" << std::endl;
            fry_timer_->expires_from_now(5s);
            fry_timer_->async_wait(net::bind_executor(strand, [start_time, self](boost::system::error_code error){
            self->OnTimer(error, start_time);
            }));
        });
    };

    void OnTimer(boost::system::error_code error, std::chrono::_V2::steady_clock::time_point start_time) {
        std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Sausage #" << id_ << " timer expired" << std::endl;
        StopFry(start_time);
    } 

    void StopFry(std::chrono::_V2::steady_clock::time_point start_time) {
        std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Sausage #" << id_ << " stopped" << std::endl;
        if (!frying_start_time_) {
            throw std::logic_error("Frying has not started");
        }
        if (frying_end_time_) {
            throw std::logic_error("Frying has already stopped");
        }
        frying_end_time_ = Clock::now();
        // Освобождаем горелку
        std::osyncstream(std::cout) << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Burner released: sausage #" << this->GetId()
                                        << std::endl;
        gas_cooker_lock_.Unlock();
        SetSausageCooked();
    }

    bool IsCooked() const noexcept {
        return frying_start_time_.has_value() && frying_end_time_.has_value();
    }

    bool IsCookedBool() const noexcept {
        return sausage_cooked_;
    }

    void SetSausageCooked() {
        sausage_cooked_ = true;
    }

private:
    int id_;
    net::io_context& io_;
    bool sausage_cooked_ = false;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    boost::asio::steady_timer fry_timer{io_};
    GasCookerLock gas_cooker_lock_;
    std::optional<Clock::time_point> frying_start_time_;
    std::optional<Clock::time_point> frying_end_time_;

};

class HotDog{
public:
    HotDog(int id, std::shared_ptr<Sausage> sausage) :
        id_{id},
        sausage_{std::move(sausage)} {
    }

    int GetId() const noexcept {
        return id_;
    }

    Sausage& GetSausage() noexcept {
        return *sausage_;
    }

    const Sausage& GetSausage() const noexcept {
        return *sausage_;
    }
    
private:
    int id_;
    std::shared_ptr<Sausage> sausage_;
};

class Store {
public:
    std::shared_ptr<Sausage> GetSausage(net::io_context& io) {
        return std::make_shared<Sausage>(++next_id_, io);
    }
private:
    int next_id_ = 0;
};


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

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(int id, net::io_context& io, std::shared_ptr<HotDog> hot_dog) :
        id_{id},
        io_{io},
        hot_dog_{std::move(hot_dog)} {
    }
    
    void Execute(std::shared_ptr<GasCooker> gas_cooker, std::chrono::_V2::steady_clock::time_point start_time) {
        hot_dog_->GetSausage().StartFry(*gas_cooker, start_time, [](){});
    }
private:
    int id_;
    net::io_context& io_;
    std::shared_ptr<HotDog> hot_dog_;
};

class Cafeteria {
public: 
    explicit Cafeteria(net::io_context& io) :
        io_{io} {
    }
    void OrderHotDog(int id, std::chrono::_V2::steady_clock::time_point start_time, std::function<void(Result<HotDog> hot_dog)> handler) {
        std::osyncstream{std::cout} << std::chrono::duration<double>(steady_clock::now() - start_time).count() << "> Hot dog #" << id << " ordered" <<std::endl;
        auto sausage = store_.GetSausage(io_);
        HotDog hot_dog(id, sausage);
        std::make_shared<Order>(id, io_, std::make_shared<HotDog>(hot_dog))->Execute(gas_cooker_, start_time);
        handler(hot_dog);
    }

    
private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};

void PrintHotDogResult(const Result<HotDog>& result) {
    if (result.HasValue()) {
        auto& hot_dog = result.GetValue();
        std::osyncstream{std::cout} << "Hot dog #" << hot_dog.GetId()
                                    << " sausage ready: " << hot_dog.GetSausage().IsCookedBool()
                                    << std::endl;
    } else {
        throw std::logic_error("Error in Result");
    }
}

void PrepareHotDogs(int num_orders, unsigned num_threads) {
    net::io_context io(static_cast<int>(num_threads));
    Cafeteria cafeteria(io);
    std::mutex mut;
    auto num_waiting_threads = std::min<int>(num_threads, num_orders);

    const auto start_time = steady_clock::now();

    std::latch start(num_waiting_threads);

    for (int i = 0; i < num_orders; ++i) {
        net::dispatch(io, [&io, start_time, &cafeteria, i, &start, num_waiting_threads]{
            std::osyncstream{std::cout} << "Order #" << i << " is scheduled on thread #"
                                        << std::this_thread::get_id() << std::endl;
            if (i < num_waiting_threads) {
                start.arrive_and_wait();
            }
            cafeteria.OrderHotDog(i, start_time, [i](Result<HotDog> result){
                //std::osyncstream{std::cout} << "HotDog Handler #" << i << std::endl;
                //PrintHotDogResult(result);
                });
            });
    }

    RunWorkers(num_threads, [&io] {
        io.run();
    });
}

int main() {
    using namespace std::chrono;
    constexpr unsigned num_threads = 2;
    constexpr int num_orders = 4;

    PrepareHotDogs(num_orders, num_threads);
}