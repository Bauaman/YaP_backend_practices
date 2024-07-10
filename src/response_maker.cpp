#include "response_maker.h"

namespace http_handler {

http::response<http::string_body> MakeResponse(http::status status, std::string_view text, 
                                    unsigned version, bool keep_alive, 
                                    std::string_view content_type,
                                    std::string_view cache,
                                    std::string_view allow) {

    http::response<http::string_body> response(status, version);
    response.set(http::field::content_type, content_type);
    response.body() = text;
    response.prepare_payload();
    response.keep_alive(keep_alive);
    if (!cache.empty()) {
        response.set(http::field::cache_control, cache);
    }
    if (!allow.empty()) {
        response.set(http::field::allow, allow);
    }
    return response;
}

http::response<http::file_body> MakeResponse(http::status status, http::file_body::value_type& file, 
                                    unsigned version,
                                    bool keep_alive,
                                    std::string_view content_type, std::string_view cache_control) {
    http::response<http::file_body> response(status, version);
    response.set(http::field::content_type, content_type);
    response.body() = std::move(file);
    response.prepare_payload();
    response.keep_alive(keep_alive);
    if (!cache_control.empty()) {
        response.set(http::field::cache_control, cache_control);
    }
    return response;
}

}