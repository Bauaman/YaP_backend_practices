#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include <syncstream>
#include <string>
#include <string_view>

using namespace std::literals;
using namespace std::chrono;

namespace net = boost::asio;
namespace sys = boost::system;

class Logger {
public:
    explicit Logger(std::string id) :
        id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now()-start_time_).count() 
           << "s]"sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const {
        return cutlet_roasted_;
    }

    void SetCutletRoasted() {
        if (IsCutletRoasted()) {
            throw std::logic_error("Cutlet has been roasted already"s);
        }
        cutlet_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }

    void AddOnion() {
        if (IsPacked()) {
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureCutletRoasted();
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }

    void Pack() {
        AssureCutletRoasted();
        is_packed_ = true;
    }

private:
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            throw std::logic_error("Cutlet has not been roasted yet"s);
        }
    }

    bool cutlet_roasted_ = false;
    bool has_onion_ = false;
    bool is_packed_ = false;
};

using OrderHandler = std::function<void(sys::error_code error, int id, Hamburger* hamburger)>;

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, int id, bool with_onion, OrderHandler handler) :
        io_{io},
        id_{id},
        with_onion_{with_onion},
        handler_{std::move(handler)} {

        }

    void Execute(){
        logger_.LogMessage("Order has been started."sv);
        RoastCutlet();
        if (with_onion_) {
            MarinadeOnion();
        }
    }
private:

    void RoastCutlet(){
        logger_.LogMessage("Start roasting cutlet."sv);
        roast_timer_.async_wait([self = shared_from_this()](sys::error_code error){
            self->OnRoasted(error);
        });
    }
    void OnRoasted(sys::error_code error) {
        if (error) {
            logger_.LogMessage("Roast error: "s + error.what());
        } else {
            hamburger_.SetCutletRoasted();
            logger_.LogMessage("Ended roasting cutlet"sv);
        }
        CheckReadiness(error);
    }

    void MarinadeOnion(){
        logger_.LogMessage("Start marinading onions"sv);
        marinade_timer_.async_wait([self = shared_from_this()](sys::error_code error) {
            self->OnMarinaded(error);
        });
    }

    void OnMarinaded(sys::error_code error) {
        if (error) {
            logger_.LogMessage("Marinade error: "s + error.what());
        } else {
            onion_marinaded_ = true;
            logger_.LogMessage("Ended marinading onions"sv);
        }
        CheckReadiness(error);
    }

    void CheckReadiness(sys::error_code error) {
        logger_.LogMessage(error.to_string());
    }

    net::io_context& io_;
    int id_;
    bool with_onion_;
    OrderHandler handler_;
    Logger logger_{std::to_string(id_)};
    Hamburger hamburger_;
    bool onion_marinaded_ = false;
    boost::asio::steady_timer roast_timer_{io_, 1s};
    boost::asio::steady_timer marinade_timer_{io_, 2s};
};

class Resturant {
public:
    explicit Resturant(net::io_context& io) :
        io_(io) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id;
        std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();
        return order_id;
    }
private:
    net::io_context& io_;
    int next_order_id = 0;
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsCutletRoasted() ? "roasted cutlet"sv : "raw cutlet"sv)
              << (h.HasOnion() ? "onion ready"sv : "no onion"sv)
              << (h.IsPacked() ? "packed"sv : "not packed"sv);
}

int main() {
    net::io_context io;
    Resturant restaurant(io);
    restaurant.MakeHamburger(true,[](sys::error_code error, int order_id, Hamburger* h){});
    io.run();
}