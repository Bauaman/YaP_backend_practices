#include "json_loader.h"

using namespace std::literals;

namespace json_loader {

using namespace strconsts;

std::filesystem::path operator""_p(const char* data, std::size_t sz) {
    return std::filesystem::path(data, data + sz);
} 

std::string LoadJsonFileAsString(const std::filesystem::path& json_path) {
    std::ifstream jsonfile;
    std::filesystem::path filepath = json_path;
    jsonfile.open(filepath);
    
    if (!jsonfile.is_open()) {
        std::string error_message = "Failed to open file: "s + std::string(json_path);
        throw std::runtime_error("Failed to open file");
    }

    std::stringstream buffer;
    buffer << jsonfile.rdbuf();
    jsonfile.close();
    return buffer.str();
}

void SetKeySequenceRoad(const boost::json::value& road, model::Road& road_) {
    for (const auto& pair : road.as_object()) {
        road_.SetKeySequence(pair.key_c_str());
    }
}

void AddRoadsToMap(const boost::json::value& parsed, model::Map& map) {
    for (auto& road : parsed.as_array()) {
        if (road.as_object().contains(x_end)) {
            model::Road road_{model::Road::HORIZONTAL, 
                    {static_cast<int>(road.as_object().at(x_start).as_int64()), static_cast<int>(road.as_object().at(y_start).as_int64())},
                    static_cast<int>(road.as_object().at(x_end).as_int64())};
            SetKeySequenceRoad(road, road_);
            map.AddRoad(road_);
        }
        if (road.as_object().contains("y1")) {
            model::Road road_{model::Road::VERTICAL, 
                    {static_cast<int>(road.as_object().at(x_start).as_int64()), static_cast<int>(road.as_object().at(y_start).as_int64())},
                    static_cast<int>(road.as_object().at(y_end).as_int64())};
            SetKeySequenceRoad(road, road_);
            map.AddRoad(road_);
        }
    }
}

void AddBuildingsToMap(const boost::json::value& parsed, model::Map& map) {
    for (auto& building : parsed.as_array()) {
        model::Rectangle rect{{static_cast<int>(building.as_object().at(x_str).as_int64()), static_cast<int>(building.as_object().at(y_str).as_int64())},
                              {static_cast<int>(building.as_object().at(w_str).as_int64()), static_cast<int>(building.as_object().at(h_str).as_int64())}};
        model::Building building_{rect};
        for (const auto& pair : building.as_object()) {
            building_.SetKeySequence(pair.key_c_str());
        }
        map.AddBuilding(building_);
    }
}

void AddOfficesToMap(const boost::json::value& parsed, model::Map& map) {
    for (auto& office : parsed.as_array()) {
        model::Office::Id id{office.as_object().at("id").as_string().c_str()};
        model::Office office_{id, 
                              {static_cast<int>(office.as_object().at(x_str).as_int64()), static_cast<int>(office.as_object().at(y_str).as_int64())}, 
                              {static_cast<int>(office.as_object().at(x_offset).as_int64()), static_cast<int>(office.as_object().at(y_offset).as_int64())}};
        for (const auto& pair : office.as_object()) {
            office_.SetKeySequence(pair.key_c_str());
        } try {
            map.AddOffice(office_);
        } catch (std::invalid_argument& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }
}

void AddMapsToGame (const boost::json::value& parsed, model::Game& game) {
    for (auto& map : parsed.as_array()) {
        model::Map::Id id{map.as_object().at("id").as_string().c_str()};
        model::Map map_i = model::Map{id, map.as_object().at("name").as_string().c_str()};
        for (const auto& pair : map.as_object()) {
            map_i.SetKeySequence(pair.key_c_str());
            if (pair.key() == "roads") {
                AddRoadsToMap(map.as_object().at("roads").as_array(), map_i);
            }
            if (pair.key() == "buildings") {
                AddBuildingsToMap(map.as_object().at("buildings").as_array(), map_i);
            }
            if (pair.key() == "offices") {
                AddOfficesToMap(map.as_object().at("offices").as_array(), map_i);
            }
        }
        game.AddMap(map_i);
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    try {
        std::string json_as_string = LoadJsonFileAsString(json_path);
        boost::json::value parsed_json = boost::json::parse(json_as_string);

        model::Game game;

        AddMapsToGame(parsed_json.as_object().at("maps").as_array(), game);
        return game;

    } catch (const std::exception& e) {
        throw e;
    }
}

}  // namespace json_loader
