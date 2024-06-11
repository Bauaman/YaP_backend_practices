#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

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
        LogRequest(req, address_string);
        ResponseVariant response = decorated_(std::move(req)/*, std::forward<Send>(send)*/, root_dir);
        LogResponse(response);
        send(response);
    }

private:
    RequestHandler& decorated_;

    template <typename Body, typename Allocator>
    void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const std::string& address_string) {
        // Здесь вы можете добавить логирование запроса
        //std::cout << "Received request: " << req.target() << std::endl;
        boost::json::object add_data;
        add_data["ip"] = address_string;
        add_data["URI"] = std::string(req.target());
        //beast::string_view method_sv = http::to_string(req.method());
        std::string method_str(req.method_string());
        add_data["method"] = method_str;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "request received";
    }

    template <typename Body, typename Fields>
    void LogResponse(const http::response<Body, Fields>& resp) {
        // Здесь вы можете добавить логирование ответа
        //std::cout << "Sending response with status code: " << resp.result_int() << std::endl;
        boost::json::object add_data;
        add_data["code"] = resp.result_int();
        auto content_type = resp.find(http::field::content_type);
        if (content_type != resp.end()) {
            add_data["content_type"] = std::string(content_type->value());
        } else {
            add_data["content_type"] = "null";
        }
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "response sent";
    }

    void LogResponse(const ResponseVariant& response) {
        std::visit([this](const auto& resp) {
            LogResponse(resp);
        }, response);
    }
};

class RequestHandler {

    using ResponseVariant = std::variant<http::response<http::string_body>, http::response<http::file_body>>;

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

    bool IsSubPath(fs::path base, fs::path path) {
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    std::string_view ExtensionToContentType(fs::path filepath) {
        std::string extension = filepath.extension().string();
        if (extension == ".htm" || extension == ".html")
                return ContentType::TEXT_HTML;
            else if (extension == ".json")
                return ContentType::JSON;
            else if (extension == ".css")
                return ContentType::CSS;
            else if (extension == ".txt")
                return ContentType::PLAIN;
            else if (extension == ".js")
                return ContentType::JAVASCRIPT;
            else if (extension == ".xml")
                return ContentType::XML;
            else if (extension == ".png")
                return ContentType::PNG;
            else if (extension == ".jpg" || extension == ".jpeg" || extension == ".jpe")
                return ContentType::JPG;
            else if (extension == ".gif")
                return ContentType::GIF;
            else if (extension == ".bmp")
                return ContentType::BMP;
            else if (extension == ".ico")
                return ContentType::ICO;
            else if (extension == ".tiff" || extension == ".tif")
                return ContentType::TIFF;
            else if (extension == ".svg" || extension == ".svgz")
                return ContentType::SVG;
            else if (extension == ".mp3")
                return ContentType::MP3;
            else
                return ContentType::UNKNOWN;
    }

    boost::json::value PrepareAPIResponce(const std::string& req_,const model::Game& game_);

    void ReadStaticFile(const std::string& filepath, http::response<http::file_body>& response) {
        beast::error_code ec;
        http::file_body::value_type file;
        file.open(filepath.c_str(), beast::file_mode::read, ec);

        if (ec) {
            throw std::logic_error("Failed to open file: " + filepath);
        }
        
        //std::cout << "ExtensionToContentType: " << ExtensionToContentType(filepath) << std::endl;
        response.body() = std::move(file);
        response.set(http::field::content_type, ExtensionToContentType(filepath));
        response.content_length(response.body().size());
        response.keep_alive(true);
    }

    void PrepareResponseBadRequestInvalidMethod(http::response<http::string_body>& response) {
        if (response.result() == http::status::bad_request) {
            if (response.find(http::field::content_type)->value() == ContentType::JSON) {
                boost::json::value json_arr = {
                    {"code" , "badRequest"},
                    {"message" , "Bad Request"}
                };
                response.body() = boost::json::serialize(json_arr);
            }
            if (response.find(http::field::content_type)->value() == ContentType::PLAIN) {
                response.body() = "Bad Request: Requested file is outside of the root directory";
            }
            response.content_length(response.body().size());
        }
        if (response.result() == http::status::method_not_allowed) {
            response.body() = "Invalid method";
            response.content_length(response.body().size());
            response.set(http::field::allow, "GET, HEAD"sv);
        }
        
    }
    //template <typename Send>
    /*void*/ ResponseVariant HandleStaticFileRequest(/*Send&& send, */const http::request<http::basic_string_body<char>>& req, const RequestData& req_data, const std::string& root_dir) {
        try {
            std::string normalized_path = fs::weakly_canonical(fs::path(req_data.req_data)).string();
            //std::cout << "normalized path: " << normalized_path << std::endl;
            std::string filepath = root_dir + normalized_path;
            //std:: cout << "filepath: " << filepath << std::endl;
            if (fs::is_directory(filepath)) {
                filepath = filepath + "index.html";
                //std::cout << "Path to dir: " << filepath << std::endl;
            }
            if (!IsSubPath(fs::path(root_dir), filepath)) {
                http::response<http::string_body> response(http::status::bad_request, req.version());
                response.set(http::field::content_type, ContentType::PLAIN);
                PrepareResponseBadRequestInvalidMethod(response);
                response.keep_alive(req.keep_alive());
                response.prepare_payload();
                
                return response;
                //send(std::move(response));
            } else {
                http::response<http::file_body> response(http::status::ok, req.version());
                ReadStaticFile(filepath, response);
                
                //send(std::move(response));
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

            //send(std::move(response));
            return response;
        }
    }

    //template <typename Send>
    /*void*/http::response<http::string_body> HandleAPIRequest(/*Send&& send, */const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
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
        //send(std::move(response));
    }

    template <typename Body, typename Allocator/*, typename Send*/>
    /*void*/ResponseVariant operator()(http::request<Body, http::basic_fields<Allocator>>&& req/*, Send&& send*/, const std::string& root_dir) {
        // Обработать запрос request и отправить ответ, используя send
        auto data = req.body();
        RequestData req_;
        std::string req_target = std::string(req.target());
        ResponseVariant response_v;
        if ((req.method() == http::verb::get) || (req.method() == http::verb::head)) {
            try {
                req_ = RequestParser(req_target);
                if (req_.req_type == RequestType::API) {
                    response_v = HandleAPIRequest(/*std::forward<Send>(send),*/ req, req_);
                }
                if (req_.req_type == RequestType::STATIC_FILE) {
                    response_v = HandleStaticFileRequest(/*std::forward<Send>(send),*/ req, req_, root_dir);
                }
            } catch (std::logic_error& ex) {
                http::response<http::string_body> resp(http::status::bad_request, req.version());
                resp.set(http::field::content_type, ContentType::JSON);
                PrepareResponseBadRequestInvalidMethod(resp);
                resp.keep_alive(req.keep_alive());
                resp.prepare_payload();
                //send(std::move(resp));
                return resp;
            }
        } else {
            http::response<http::string_body> resp(http::status::method_not_allowed, req.version());
            resp.set(http::field::content_type, ContentType::JSON);
            PrepareResponseBadRequestInvalidMethod(resp);
            resp.keep_alive(req.keep_alive());
            resp.prepare_payload();
            //send(std::move(resp));
            return resp;
        }
        return response_v;
    }

private:
    model::Game& game_;
};

}  // namespace http_handler


