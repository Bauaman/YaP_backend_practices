#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <chrono>
#include <filesystem>
#include <iostream>
#include "http_server.h"
#include "model.h"

//#include "logger.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace logging = boost::log;

using namespace std::literals;


enum class RequestType {
    API,
    STATIC_FILE
};

struct RequestData {
    RequestType req_type;
    std::string req_data;
};

std::string UrlDecode(const std::string& str);
boost::json::value PrepareOfficesForResponce(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareRoadsForResponse(const model::Map& map);
RequestData RequestParser(const std::string& req_target);

template<typename RequestHandler>
class LoggingRequestHandler {

using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

public:

    LoggingRequestHandler(RequestHandler& decorated) :
        decorated_(decorated) {
        }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& root_dir, const std::string& address_string) {
        std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
        LogRequest(req, address_string);
        ResponseVariant response = PrepareResponse(std::move(req), root_dir);
        std::chrono::system_clock::time_point end_time = std::chrono::system_clock::now();
        auto response_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        LogResponse(response, address_string, response_time);
        send(response);


    }

private:
    RequestHandler& decorated_;

    template <typename Body, typename Allocator>
    void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& address_string) {
        boost::json::object add_data;
        add_data["ip"] = address_string;
        add_data["URI"] = std::string(req.target());
        std::string method_str(req.method_string());
        add_data["method"] = method_str;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "request received";
    }

    template <typename Body, typename Fields>
    void LogResponse(const http::response<Body, Fields>& resp, const std::string& address_string, long long resp_time) {
        boost::json::object add_data;
        add_data["ip"] = address_string;
        add_data["response_time"] = resp_time;
        add_data["code"] = resp.result_int();
        auto content_type = resp.find(http::field::content_type);
        if (content_type != resp.end()) {
            add_data["content_type"] = std::string(content_type->value());
        } else {
            add_data["content_type"] = "null";
        }
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "response sent";
    }

    void LogResponse(const ResponseVariant& response, const std::string& address_string, long long resp_time) {
        std::visit([this, address_string, resp_time](const auto& resp) {
            LogResponse(resp, address_string, resp_time);
        }, response);
    }

    template <typename Body, typename Allocator>
    ResponseVariant PrepareResponse(http::request<Body, http::basic_fields<Allocator>>&& req, const std::string& root_dir) {
        return decorated_(std::move(req), root_dir);
    }
};

class RequestHandler {

    using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;
    using SendFunction = std::function<void(ResponseVariant)>;

public:
    explicit RequestHandler(model::Game& game, fs::path root) :
        game_{game},
        root_{root} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view JSON = "application/json"sv;
        constexpr static std::string_view CSS = "text/css"sv;
        constexpr static std::string_view PLAIN = "text/plain"sv;
        constexpr static std::string_view JAVASCRIPT = "text/javascript"sv;
        constexpr static std::string_view XML = "application/xml"sv;
        constexpr static std::string_view PNG = "image/png"sv;
        constexpr static std::string_view JPG = "image/jpeg"sv;
        constexpr static std::string_view GIF = "image/gif"sv;
        constexpr static std::string_view BMP = "image/bmp"sv;
        constexpr static std::string_view ICO = "image/vnd.microsoft.icon"sv;
        constexpr static std::string_view TIFF = "image/tiff"sv;
        constexpr static std::string_view SVG = "image/svg+xml"sv;
        constexpr static std::string_view MP3 = "audio/mpeg"sv;
        constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
    };

    template <typename Body, typename Allocator>
    ResponseVariant operator()(http::request<Body, http::basic_fields<Allocator>>&& req, const std::string& root_dir) {
        auto data = req.body();
        RequestData req_;
        std::string req_target = std::string(req.target());
        ResponseVariant response_v;
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                if (req_.req_type == RequestType::API) {
                    response_v = HandleAPIRequest(req, req_);
                }
                if (req_.req_type == RequestType::STATIC_FILE) {
                    response_v = HandleStaticFileRequest(req, req_, root_dir);
                }
            } catch (std::logic_error& ex) {
                http::response<http::string_body> resp(http::status::bad_request, req.version());
                resp.set(http::field::content_type, ContentType::JSON);
                PrepareResponseBadRequestInvalidMethod(resp);
                resp.keep_alive(req.keep_alive());
                resp.prepare_payload();
                return resp;
            }
        } else {
            http::response<http::string_body> resp(http::status::method_not_allowed, req.version());
            resp.set(http::field::content_type, ContentType::JSON);
            PrepareResponseBadRequestInvalidMethod(resp);
            resp.keep_alive(req.keep_alive());
            resp.prepare_payload();
            return resp;
        }
        return response_v;
    }

private:
    model::Game& game_;
    fs::path root_;

    bool IsSubPath(fs::path base, fs::path path);
    std::string_view ExtensionToContentType(fs::path filepath);
    boost::json::value PrepareAPIResponce(const std::string& req_,const model::Game& game_);
    void ReadStaticFile(fs::path& filepath, http::response<http::file_body>& response);
    void PrepareResponseBadRequestInvalidMethod(http::response<http::string_body>& response);

    ResponseVariant HandleStaticFileRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
        try {
            fs::path req_path = fs::path(req_data.req_data);
            fs::path filepath = fs::path(root_.string() + "/" + req_path.string());
            if (fs::is_directory(filepath)) {
                filepath = filepath / "index.html";
            }
            if (!IsSubPath(fs::path(root_), filepath)) {
                http::response<http::string_body> response(http::status::bad_request, req.version());
                response.set(http::field::content_type, ContentType::PLAIN);
                PrepareResponseBadRequestInvalidMethod(response);
                response.keep_alive(req.keep_alive());
                response.prepare_payload();
                
                return response;
            } else {
                http::response<http::file_body> response(http::status::ok, req.version());
                ReadStaticFile(filepath, response);
                
                return response;
            }
        } catch (const std::exception& ex) {
            http::response<http::string_body> response(
            http::status::not_found, req.version());
            response.set(http::field::content_type, ContentType::PLAIN);
            response.keep_alive(req.keep_alive());
            response.body() = "File not found: " + req_data.req_data;
            response.content_length(response.body().size());
            response.prepare_payload();

            return response;
        }
    }

    http::response<http::string_body> HandleAPIRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
        http::response<http::string_body> response(http::status::ok, req.version());
        boost::json::value response_body = PrepareAPIResponce(req_data.req_data, game_);
        if (response_body.is_object()) {
            if (response_body.as_object().find("code") != response_body.as_object().end() && 
                response_body.as_object().at("code") == "mapNotFound") {
                response.result(http::status::not_found);
            }
        }
        response.body() = boost::json::serialize(response_body);
        response.keep_alive(req.keep_alive());
        response.set(http::field::content_type, ContentType::JSON);
        response.content_length(response.body().size());
        response.prepare_payload();
        
        return response;
        
    }

};

}  // namespace http_handler


