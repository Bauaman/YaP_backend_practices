#pragma once

#include <random>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace std::literals;

namespace http_handler {

struct Errors {
Errors() = delete;
    constexpr static std::string_view MAP_NOT_FOUND = R"({"code": "mapNotFound", "message": "Map not found"})"sv;
    constexpr static std::string_view BAD_REQ = R"({"code": "badRequest", "message": "Bad request"})"sv;
    constexpr static std::string_view PARSING_ERROR = R"({"code": "invalidArgument", "message": "Join request parsing failed"})"sv;
    constexpr static std::string_view ACTION_PARSING_ERROR = R"({"code": "invalidArgument", "message": "Action request parsing failed"})"sv;
    constexpr static std::string_view USERNAME_EMPTY = R"({"code": "invalidArgument", "message": "Invalid name"})"sv;
    constexpr static std::string_view POST_INVALID = R"({"code": "invalidMethod", "message": "Only POST method is expected"})"sv;
    constexpr static std::string_view GET_INVALID = R"({"code": "invalidMethod", "message": "Only GET method is expected"})"sv;
    constexpr static std::string_view INVALID_METHOD = R"({"code": "invalidMethod", "message": "GET or HEAD method is expected"})"sv;
    constexpr static std::string_view AUTH_HEADER = R"({"code": "invalidToken", "message": "No authorization header in request"})"sv;
    constexpr static std::string_view INVALID_HEADER = R"({"code": "invalidToken", "message": "No authorization header is invalid"})"sv;
    constexpr static std::string_view INVALID_TOKEN = R"({"code": "invalidToken", "message": "Player token is invalid"})"sv;
    constexpr static std::string_view UNKNOWN_TOKEN = R"({"code": "unknownToken", "message": "Player token not found"})"sv;
};

struct ContentType {
ContentType() = delete;
    constexpr static std::string_view HTML = "text/html"sv;
    constexpr static std::string_view PLAIN = "text/plain"sv;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
    static const std::unordered_map<std::string, std::string_view> DICT;
};

}

namespace auxillary {

std::string UrlDecode(const std::string& str);
bool IsSubPath(fs::path base, fs::path path);
int GetRandomNumber(int min, int max);

}