#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <iostream>
#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view JSON = "application/json"sv;
    };

    boost::json::value PrepareResponce(const std::string& req_, model::Game& game_);

    template <typename Body, typename Allocator>
    std::string RequestParser(const http::request<Body, http::basic_fields<Allocator>>& req) {
        std::string target = std::string(req.target());
        std::cout << "Target: " << target << std::endl;
        size_t pos = target.find("/api/v1/") + 7;
        std::cout << pos << std::endl;
        if (pos != 6) { //проверка на правильный префикс
            std::string req_ = target.substr(pos+1);
            size_t next_slash_pos = req_.find('/');

            if (next_slash_pos != req_.length() && next_slash_pos != std::string::npos) {
                std::cout << "Next slash pos: " << next_slash_pos << std::endl;
                req_ = req_.substr(next_slash_pos+1);
                std::cout << "Req_ 2 " << req_ << std::endl;
                if (req_.find('/') == std::string::npos) {
                    std::cout << "Here 2" << " " << req_ << std::endl;
                    return std::string(req_);
                } else {
                    throw std::logic_error("Invalid request (/api/v1/maps/id/?)"s);
                }
            } else {
                if (req_ == "maps") {
                    std::cout << "Return here" << std::endl;
                    return std::string(req_);
                } else {
                    throw std::logic_error("Invalid request (/api/v1/?)"s);
                }
            }
        } else {
            throw std::logic_error("Invalid request (/api/?)"s);
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        http::status status;
        std::string text;
        std::string req_;
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req);
                std::cout << "After parser: " << req_ << std::endl;
                boost::json::value response_body = PrepareResponce(req_, game_);

                status = http::status::ok;

                
                if (response_body.is_object()) {
                    if (response_body.as_object().find("code") != response_body.as_object().end() && 
                        response_body.as_object().at("code") == "mapNotFound") {
                        status = http::status::not_found;
                    }
                } else {
                    status = http::status::ok;
                }
                text = boost::json::serialize(response_body);
            } catch (std::logic_error& ex) {
                std::cout << "catched ex " << ex.what() << "from Request Parser" << std::endl;
                status = http::status::bad_request;
                boost::json::value jsonArr = {
                    {"code", "badRequest"},
                    {"message", "Bad request"}
                };
                text = boost::json::serialize(jsonArr);
            }
        } else {
            status = http::status::method_not_allowed;
            text = "Invalid method";
        }
        http::response<http::string_body> response(status, req.version());
        if (status == http::status::method_not_allowed) {
            response.set(http::field::allow, "GET, HEAD"sv);
        }
        response.set(http::field::content_type, ContentType::JSON);
        response.body() = text;
        response.content_length(text.size());
        response.keep_alive(req.keep_alive());
        response.prepare_payload();
        
        send(std::move(response));
    }

private:
    model::Game& game_;
};


}  // namespace http_handler


