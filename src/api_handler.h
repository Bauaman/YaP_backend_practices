#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <optional>

#include "aux.h"
#include "game_server.h"
#include "response_maker.h"
#include "tagged.h"

namespace json = boost::json;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

namespace http_handler {

boost::json::value PrepareRoadsForResponse(const model::Map& map);
boost::json::value PrepareBuildingsForResponce(const model::Map& map);
boost::json::value PrepareOfficesForResponce(const model::Map& map);

template <typename Body, typename Allocator, typename Send>
class ApiHandler {
public:
    ApiHandler(http::request<Body, http::basic_fields<Allocator>>& req, GameServer& gs, RequestData& rd) :
        req_(req),
        gs_(gs),
        r_data_(rd) {}

    http::response<http::string_body> HandleRequest() {
        try {
            if (r_data_.type == RequestType::API) {
                return HandleMapRequest();
            }
            if (r_data_.type == RequestType::PLAYER) {
                if (r_data_.r_target == "players") {
                    return HandlePlayersListRequest();
                } else if (r_data_.r_target == "join") {
                    return HandlePlayerJoinRequest();
                } else if (r_data_.r_target == "state") {
                    return HandleStateRequest();
                } else {
                    return MakeResponse(http::status::bad_request, Errors::BAD_REQ, req_.version(), req_.keep_alive(), ContentType::JSON);
                }
            } 
        }
    }

// Methods, no authorization required ->

    http::response<http::string_body> HandleMapRequest() {
        json::value message;
        if (req_.method() != http::verb::get) {
            return MakeResponse(http::status::method_not_allowed, Errors::GET_INVALID, req_.version(), req_.keep_alive(), ContentType::JSON, ""sv, "GET"sv);
        }
        
        if (r_data_.r_target.empty()) {
            return MakeResponse(http::status::bad_request, Errors::BAD_REQ, req_.version(), req_.keep_alive(), ContentType::JSON);
        }
        if (r_data_.r_target == "maps") {
            json::array array;
            for (auto& map : gs_.GetMaps()) {
                json::value val = {{"id",*map.GetId()}, {"name", map.GetName()}};
                array.push_back(val);
            }
            message = array;
            return MakeResponse(http::status::ok, json::serialize(message), req_.version(), req_.keep_alive(), ContentType::JSON); 
        } else {
            json::object resp_message;
            model::Map::Id id_{r_data_.r_target};
            const model::Map* map = gs_.FindMap(id_);
            if (map != nullptr) {
                std::vector<std::string> keys_in_map = map->GetKeys();
                for (const auto& key : keys_in_map) {
                    if (key == "id") {
                        resp_message["id"] = *map->GetId();
                    } else if (key == "name") {
                        resp_message["name"] = map->GetName();
                    } else if (key == "roads") {
                        boost::json::array roads = PrepareRoadsForResponse(*map).as_array();
                        resp_message["roads"] = roads;
                    } else if(key == "buildings") {
                        boost::json::array buildings = PrepareBuildingsForResponce(*map).as_array();
                        resp_message["buildings"] = buildings;
                    } else if (key == "offices") {
                        boost::json::array offices = PrepareOfficesForResponce(*map).as_array();
                        resp_message["offices"] = offices;            
                    }
                }
                message = resp_message;
                return MakeResponse(http::status::ok, json::serialize(message), req_.version(), req_.keep_alive(), ContentType::JSON);
                
            } else {
                return MakeResponse(http::status::not_found, Errors::MAP_NOT_FOUND, req_.version(), req_.keep_alive(), ContentType::JSON);
                
            }
        }
    }

    http::response<http::string_body> HandlePlayerJoinRequest() {
        if (req_.method() != http::verb::post) {
            return MakeResponse(http::status::method_not_allowed, Errors::POST_INVALID, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv, "POST"sv);
        }
        std::string user_name;
        std::string map_id;
        try {
            boost::json::value parsed_req = boost::json::parse(req_.body());
            if (parsed_req.as_object().find("userName") == parsed_req.as_object().end() || parsed_req.as_object().at("userName").as_string().empty()) {
                return MakeResponse(http::status::bad_request, Errors::USERNAME_EMPTY, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
            }
            user_name = parsed_req.as_object().at("userName").as_string();
            map_id = parsed_req.as_object().at("mapId").as_string();
        } catch (...) {
            return MakeResponse(http::status::bad_request, Errors::PARSING_ERROR, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
        }
        model::Map::Id mapId(map_id);
        auto map = gs_.FindMap(mapId);
        if (map == nullptr) {
            return MakeResponse(http::status::not_found, Errors::MAP_NOT_FOUND, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
        }
        boost::json::object resp;
        try {
            const model::Player& player = gs_.JoinGame(model::Map::Id(mapId), user_name);
            resp = {{"authToken", *player.GetPlayerToken()},
                    {"playerId", player.GetId()}};
        } catch (const std::exception& ex) {
            return MakeResponse(http::status::internal_server_error, "Join game failed: "s + ex.what(), req_.version(), req_.keep_alive(), ContentType::HTML);
        }
        return MakeResponse(http::status::ok, boost::json::serialize(resp), req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
    }

// Methods, authorization required ->

    http::response<http::string_body> HandlePlayersListRequest() {
        if (req_.method() != http::verb::get && req_.method() != http::verb::head) {
            return MakeResponse(http::status::method_not_allowed, Errors::INVALID_METHOD, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv, "GET, HEAD"sv);            
        } 
        return ExecuteAuthorized([this](model::Player& player) {
            boost::json::object resp;
            for (auto pl : gs_.GetPlayers()) {
                resp[std::to_string(pl.second.GetId())] = boost::json::object{{"name", pl.second.GetName()}};
            }
            return MakeResponse(http::status::ok, boost::json::serialize(resp), req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
        });
    }

    http::response<http::string_body> HandleStateRequest() {
        if (req_.method() != http::verb::get && req_.method() != http::verb::head) {
            return MakeResponse(http::status::method_not_allowed, Errors::INVALID_METHOD, req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv, "GET, HEAD"sv);
        }
        return ExecuteAuthorized([this](model::Player& player) {
            boost::json::object resp;
            for (auto pl : gs_.GetPlayers()) {
                if (pl.second.GetPlayersSession() == player.GetPlayersSession()) {
                    resp[std::to_string(pl.second.GetId())] = {
                        {"pos", {(double)pl.second.GetDog()->GetDogPosition().x_, (double)pl.second.GetDog()->GetDogPosition().y_}},
                        {"speed", {(double)pl.second.GetDog()->GetDogSpeed().x_, (double)pl.second.GetDog()->GetDogSpeed().y_}},
                        {"dir", pl.second.GetDog()->GetDogDirection()} 
                    };
                }
            }
            boost::json::object resp_message {{"players", resp}};
            return MakeResponse(http::status::ok, json::serialize(resp_message), req_.version(), req_.keep_alive(), ContentType::JSON, "no-cache"sv);
        }); 
    }


private:
    const http::request<Body, http::basic_fields<Allocator>>& req_;
    GameServer& gs_;
    RequestData& r_data_;

    std::optional<http::response<http::string_body>> TryExtractToken() {
        std::string authorization;
        if (req_.count(http::field::authorization)) {
            authorization = req_.at(http::field::authorization);
        } else {
            return std::nullopt;
        }
        const std::string auth_prefix = "Bearer ";
        if (authorization.starts_with(auth_prefix)) {
            authorization = authorization.substr(auth_prefix.size());
        } else {
            return std::nullopt;
        }
        std::transform(authorization.begin(), authorization.end(), authorization.begin(), 
            [](unsigned char c) {
                return std::tolower(c);
            });
        if (authorization.size() != 32 || authorization.find_first_not_of("0123456789abcdef") != std::string::npos) {
            return std::nullopt;
        }
        return model::Token(authorization);
    }

    template <typename Fn>
    http::response<http::string_body> ExecuteAuthorized(Fn&& action) {
        bool keep_alive = req_.keep_alive();
        unsigned version = req_.version(); 
        if (auto token = this->TryExtractToken()) {
            model::Player* pl = gs_.FindPlayer(*token);
            if (pl == nullptr) {
                return MakeResponse(http::status::unauthorized, Errors::UNKNOWN_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv);
            }
            return action(*pl);
        } else {
            return MakeResponse(http::status::unauthorized, Errors::INVALID_TOKEN, version, keep_alive, ContentType::JSON, "no-cache"sv);
        }
    }

};

}