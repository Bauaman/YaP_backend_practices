#include "request_handler.h"

namespace http_handler {

bool RequestHandler::IsSubPath(fs::path base, fs::path path) {
    fs::path combined_path = base / path;
    fs::path canonical_path = fs::weakly_canonical(path);

    return canonical_path.string().find(base.string()) == 0;
}

std::string_view RequestHandler::ExtensionToContentType(fs::path filepath) {
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

std::string UrlDecode(const std::string& str) {
    std::ostringstream decoded;
    std::istringstream encoded(str);
    encoded >> std::noskipws;

    char current;
    while (encoded >> current) {
        if (current == '%') {
            int hexCode;
            if (!(encoded >> std::hex >> hexCode)) {
                decoded << '%';
            } else {
                decoded << static_cast<char>(hexCode);
            }
        } else if (current == '+') {
            decoded << ' ';
        } else {
            decoded << current;
        }
    }
    return decoded.str();
}

RequestData RequestParser(const std::string& req_target) {
    std::string req_decoded = UrlDecode(req_target);
    if (req_decoded.find("/api") == 0) {
        size_t pos = req_decoded.find("/api/v1/");
        if (pos != std::string::npos) { //проверка на правильный префикс
            std::string req_ = req_decoded.substr(pos + 8);
            size_t next_slash_pos = req_.find('/');

            if (next_slash_pos != req_.length() && next_slash_pos != std::string::npos) {
                req_ = req_.substr(next_slash_pos+1);
                if (req_.find('/') == std::string::npos) {
                    return {RequestType::API, std::string(req_)};
                } else {
                    throw std::logic_error("Invalid request (/api/v1/maps/id/?)"s);
                }
            } else {
                if (req_ == "maps") {
                    return {RequestType::API,std::string(req_)};
                } else {
                    throw std::logic_error("Invalid request (/api/v1/?)"s);
                }
            }
        } else {
            throw std::logic_error("Invalid request (/api/?)"s);
        }
    } else {
        return {RequestType::STATIC_FILE, req_decoded};
    }
}

using namespace strconsts;
boost::json::value PrepareRoadsForResponse(const model::Map& map) {
    boost::json::array roads;
    for (const auto& road : map.GetRoads()) {
        boost::json::object road_;
        for (const std::string& str : road.GetKeys()) {
            if (str == x_start) {road_[x_start] = road.GetStart().x;}
            if (str == x_end) {road_[x_end] = road.GetEnd().x;}
            if (str == y_start) {road_[y_start] = road.GetStart().y;}
            if (str == y_end) {road_[y_end] = road.GetEnd().y;}
        }
        roads.push_back(road_);
    }
    return roads;
}

boost::json::value PrepareBuildingsForResponce(const model::Map& map) {
    boost::json::array buildings;
    for (const auto& building : map.GetBuildings()) {
        boost::json::object build_;
        for (const std::string& str : building.GetKeys()) {
            if (str == x_str) {build_[x_str] = building.GetBounds().position.x;}
            if (str == y_str) {build_[y_str] = building.GetBounds().position.y;}
            if (str == w_str) {build_[w_str] = building.GetBounds().size.width;}
            if (str == h_str) {build_[h_str] = building.GetBounds().size.height;}
        }
        buildings.push_back(build_);
    }
    return buildings;
}

boost::json::value PrepareOfficesForResponce(const model::Map& map) {
    boost::json::array offices;
    for (const auto& office : map.GetOffices()) {
        boost::json::object office_;
        for (const std::string& str : office.GetKeys()) {
            if (str == "id") {office_["id"] = *office.GetId();}
            if (str == x_str) {office_[x_str] = office.GetPosition().x;}
            if (str == y_str) {office_[y_str] = office.GetPosition().y;}
            if (str == x_offset) {office_[x_offset] = office.GetOffset().dx;}
            if (str == y_offset) {office_[y_offset] = office.GetOffset().dy;}
        }
        offices.push_back(office_);
    }
    return offices;
}

boost::json::value RequestHandler::PrepareAPIResponce(const std::string& req_, const model::Game& game_) {
    boost::json::array response_text_arr;
    boost::json::object response_text_obj;
    const std::vector<model::Map>& maps = game_.GetMaps();
    
    if (req_.empty()) {
        throw std::logic_error("bad request /api/v1/maps/...");
    }
    if (req_ == "maps") {
        for (auto& map : maps) {
            boost::json::object map_object;
            map_object["id"] = *map.GetId();
            map_object["name"] = map.GetName();
            response_text_arr.push_back(map_object);
        }
        return response_text_arr;
    } else {
        model::Map::Id  id_{req_};
        const model::Map* map = game_.FindMap(id_);
        if (map != nullptr) {
            std::vector<std::string> keys_in_map = map->GetKeys();
            for (const auto& key : keys_in_map) {
                if (key == "id") {
                    response_text_obj["id"] = *map->GetId();
                } else if (key == "name") {
                    response_text_obj["name"] = map->GetName();
                } else if (key == "roads") {
                    boost::json::array roads = PrepareRoadsForResponse(*map).as_array();
                    response_text_obj["roads"] = roads;
                } else if(key == "buildings") {
                    boost::json::array buildings = PrepareBuildingsForResponce(*map).as_array();
                    response_text_obj["buildings"] = buildings;
                } else if (key == "offices") {
                    boost::json::array offices = PrepareOfficesForResponce(*map).as_array();
                    response_text_obj["offices"] = offices;            
                }
            }
            return response_text_obj;
        } else {
            response_text_obj["code"] = "mapNotFound";
            response_text_obj["message"] = "Map Not Found";
            return response_text_obj;
        }
    }
    return response_text_arr;
}

void RequestHandler::ReadStaticFile(fs::path& filepath, http::response<http::file_body>& response) {
    beast::error_code ec;
    http::file_body::value_type file;
    file.open(filepath.c_str(), beast::file_mode::read, ec);
    if (ec) {
        throw std::logic_error("Failed to open file: " + filepath.string());
    }
    response.body() = std::move(file);
    response.set(http::field::content_type, ExtensionToContentType(filepath));
    response.content_length(response.body().size());
    response.keep_alive(true);
}

void RequestHandler::PrepareResponseBadRequestInvalidMethod(http::response<http::string_body>& response) {
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

RequestHandler::ResponseVariant RequestHandler::HandleStaticFileRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
    try {
        fs::path req_path = fs::path(req_data.req_data);
        fs::path filepath = fs::path(root_.string() + "/" + req_path.string());
        if (fs::is_directory(filepath)) {
            filepath = filepath / "index.html";
        }
        if (!IsSubPath(root_, filepath)) {
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

http::response<http::string_body> RequestHandler::HandleAPIRequest(const http::request<http::basic_string_body<char>>& req, const RequestData& req_data) {
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

} //namespace http_handler 


