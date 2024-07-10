#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/beast/http.hpp>

#include <string_view>
#include "aux.h"

namespace http_handler {

namespace http = boost::beast::http;
using namespace std::literals;

http::response<http::string_body> MakeResponse(http::status status, std::string_view text, 
                                    unsigned version, bool keep_alive, 
                                    std::string_view content_type = "text/html"sv,
                                    std::string_view cache = ""sv,
                                    std::string_view allow = ""sv);

http::response<http::file_body> MakeResponse(http::status status, http::file_body::value_type& file, 
                                    unsigned version,
                                    bool keep_alive,
                                    std::string_view content_type = "application/octet-stream"sv, std::string_view cache_control = "");

}