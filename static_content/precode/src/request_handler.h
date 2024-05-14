#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <iostream>
#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

enum RequestTypeEnum {
    API,
    STATIC_FILE
};

struct RequestType {
    std::string request_targer_parsed;
    RequestTypeEnum type;
};

boost::json::value PrepareOfficesForResponce(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareRoadsForResponse(const model::Map& map);
RequestType RequestParser(const std::string& req_target);

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
        constexpr static std::string_view CSS = "text/css"sv;
        constexpr static std::string_view TXT = "text/plain"sv;
        constexpr static std::string_view JS = "text/javascript"sv;
        constexpr static std::string_view XML = "application/xml"sv;
        constexpr static std::string_view PNG = "image/png"sv;
        constexpr static std::string_view JPG = "image/jpeg"sv;
        constexpr static std::string_view GIF = "image/gif"sv;
        constexpr static std::string_view BMP = "image/bmp"sv;
        constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
        constexpr static std::string_view TIFF = "image/tiff"sv;
        constexpr static std::string_view SVG = "image/svg+xml"sv;
        constexpr static std::string_view MP3 = "audio/mpeg"sv;
    };

    struct SetResponcseVisitor {
        bool keep_alive;

        void operator()(http::response<http::string_body>& resp) const {
            resp.keep_alive(keep_alive);
            resp.prepare_payload();
        }

        void operator()(http::response<http::file_body>& resp) const {

            resp.keep_alive(keep_alive);
            resp.prepare_payload();
        }
    };

    void SetResponseFields(ResponseVariant& resp, bool keep_alive) {
        SetResponcseVisitor visitor{keep_alive};
        std::visit(visitor, resp);
    }

    http::response<http::string_body> CreateResponseBadReqNotAllowed(http::status status);

    boost::json::value PrepareResponceAPI(const std::string& req_,const model::Game& game_);
    http::response<http::file_body> CreateResponseFileBody(const std::string& req_);
    http::response<http::string_body> CreateResponseStringBody(const std::string& req, const model::Game& game_);

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        ResponseVariant response;
        RequestType req_;
        std::string req_target = std::string(req.target());
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                if (req_.type == RequestTypeEnum::API) {
                    response = CreateResponseStringBody(req_.request_targer_parsed, game_);                    
                } else if (req_.type == RequestTypeEnum::STATIC_FILE) {
                    std::cout << req_.request_targer_parsed << std::endl;
                }
            } catch (std::logic_error& ex) {
                response = CreateResponseBadReqNotAllowed(http::status::bad_request);
            }
        } else {
            response = CreateResponseBadReqNotAllowed(http::status::method_not_allowed);
        }
        SetResponseFields(response, req.keep_alive());
        send(std::move(response));
    }

private:
    model::Game& game_;
};


}  // namespace http_handler


