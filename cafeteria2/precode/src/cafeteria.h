#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <syncstream>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using namespace std::literals;
// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


class Order : public std::enable_shared_from_this<Order> {
public:
    Order(int id, net::io_context& io, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, HotDogHandler handler) :
        id_{id},
        io_{io},
        sausage_{std::move(sausage)},
        bread_{std::move(bread)},
        handler_{std::move(handler)} {            
    }
    
    void Execute(std::shared_ptr<GasCooker> gas_cooker) {
        FrySausage(gas_cooker);
        BakeBread(gas_cooker);
    }

    std::shared_ptr<HotDog> GetHotDog() {
        return hd_;
    }

    bool IsReadyToPack() const {
        return ready_to_pack_;
    }

    int GetId() const {
        return id_;
    }

private:

    void FrySausage(std::shared_ptr<GasCooker> gas_cooker) {
        sausage_->StartFry(*gas_cooker, [self = shared_from_this()](){
            self->OnStartFrySausage();
        });
    }

    void OnStartFrySausage() {
        ftimer_.expires_from_now(1500ms);
        ftimer_.async_wait(net::bind_executor(strand_, [self = shared_from_this()](sys::error_code error){
            self->OnSausageFried();
        }));
    }

    void OnSausageFried() {
        sausage_->StopFry();
        CheckReadiness();
    }

    void BakeBread(std::shared_ptr<GasCooker> gas_cooker) {
        bread_->StartBake(*gas_cooker, [self = shared_from_this()](){
            self->OnStartBake();
        });
    }
    
    void OnStartBake() {
        btimer_.expires_from_now(1s);
        btimer_.async_wait(net::bind_executor(strand_, [self = shared_from_this()](sys::error_code error){
            self->OnBreadBaked();
            
        }));
    }

    void OnBreadBaked() {
        bread_->StopBaking();
        CheckReadiness();
    }

    void CheckReadiness() {
        std::osyncstream(std::cout) << "Sausage " << sausage_->GetId() << ": " << sausage_->IsCooked() << std::endl;
        std::osyncstream(std::cout) << "Bread " << bread_->GetId() << ": " << bread_->IsCooked() << std::endl;
        if (IngredientsReady()) {
            std::osyncstream(std::cout) << "Hotdog #" << id_ << " Everything ready" << std::endl;
            hd_ = std::make_shared<HotDog>(id_, sausage_, bread_);
            Deliver();
        }
    }

    [[nodiscard]] bool IngredientsReady() const {
        return sausage_->IsCooked() && bread_->IsCooked();
        
    }

    void Deliver() {
        Result<HotDog> res(*hd_);
        handler_(res);
    }

private:
    int id_;
    bool ready_to_pack_ = false;
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<HotDog> hd_;    
    boost::asio::steady_timer ftimer_{io_};
    boost::asio::steady_timer btimer_{io_};
    HotDogHandler handler_;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    void OrderHotDog(int id, HotDogHandler handler) {
            std::shared_ptr<Sausage> s = store_.GetSausage();
            std::shared_ptr<Bread> b = store_.GetBread();
            std::shared_ptr<Order> order = std::make_shared<Order>(id, io_, s, b, std::move(handler));
            order->Execute(gas_cooker_);/*
            if (order->IsReadyToPack()) {
                std::shared_ptr<HotDog> hd_ = order->GetHotDog();
                std::osyncstream(std::cout) << "Packing order #" << hd_->GetId() << std::endl;
                handler(*hd_);
            }*/
    }
    

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
