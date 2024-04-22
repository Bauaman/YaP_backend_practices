#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

//#include "ingredients.h"
#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


// Класс "Кафетерий". Готовит хот-доги

class Order : public std::enable_shared_from_this<Order> {
public:
    explicit Order(std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, HotDogHandler handler, net::io_context& io) :
        sausage_{std::move(sausage)},
        bread_{std::move(bread)},
        handler_{std::move(handler)},
        io_{io} {
            id_ = sausage_->GetId();
    }

    void Execute(std::shared_ptr<GasCooker> gas_cooker) {
        sausage_->StartFry(*gas_cooker, [self = shared_from_this()] {
                        self->OnFryingStart();
        });

        bread_->StartBake(*gas_cooker, [self = shared_from_this()] {
                        self->OnIngredientsReady();
        });
    }

private:

    void OnFryingStart() {
        std::osyncstream(std::cout) << "Frying sausage " << std::endl;
        frying_timer_.async_wait(net::bind_executor(strand_,[](std::shared_ptr<Sausage> sausage_){sausage_->StopFry();}));

    }

    void OnIngredientsReady() {
        if (sausage_->IsCooked() && bread_->IsCooked()) {
            try {
                handler_(Result<HotDog>{HotDog{id_, sausage_, bread_}});
            } catch (...) {
                handler_(Result<HotDog>{std::current_exception()});
            }
        }
    }
    int id_;
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    boost::asio::steady_timer frying_timer_{io_, 150ms};
    HotDogHandler handler_;
};

class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} { 
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        auto sausage = store_.GetSausage();
        auto bread = store_.GetBread();
        std::osyncstream(std::cout) << "OrderHotDog # " << sausage->GetId() << " " << bread->GetId() << std::endl;
        std::make_shared<Order>(std::move(sausage), std::move(bread), std::move(handler))->Execute(gas_cooker_);
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};