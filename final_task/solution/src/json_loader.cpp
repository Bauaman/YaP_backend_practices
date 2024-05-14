#include "json_loader.h"


namespace json_loader {

std::filesystem::path operator""_p(const char* data, std::size_t sz) {
    return std::filesystem::path(data, data + sz);
} 

std::string LoadJsonFileAsString(const std::filesystem::path& json_path) {
    std::ifstream jsonfile;
    std::filesystem::path filepath = /*"../../data"_p /*/ json_path;
    jsonfile.open(filepath);
    
    if (!jsonfile.is_open()) {
        std::cerr << "Failed to open file: " << json_path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << jsonfile.rdbuf();
    return buffer.str();
}

void AddRoadsToMap(const boost::json::value& parsed, model::Map& map) {
    for (auto& road : parsed.as_array()) {
        model::Road road_;
        if (road.as_object().contains("x1")) {
            road_ = {model::Road::HORIZONTAL, 
                    {static_cast<int>(road.as_object().at("x0").as_int64()), static_cast<int>(road.as_object().at("y0").as_int64())},
                    static_cast<int>(road.as_object().at("x1").as_int64())};
            
        }
        if (road.as_object().contains("y1")) {
            road_ = {model::Road::VERTICAL, 
                    {static_cast<int>(road.as_object().at("x0").as_int64()), static_cast<int>(road.as_object().at("y0").as_int64())},
                    static_cast<int>(road.as_object().at("y1").as_int64())};
        }
        for (const auto& pair : road.as_object()) {
            road_.SetKeySequence(pair.key_c_str());
        }
        map.AddRoad(road_);
    }
}

void AddBuildingsToMap(const boost::json::value& parsed, model::Map& map) {
    for (auto& building : parsed.as_array()) {
        model::Rectangle rect{{static_cast<int>(building.as_object().at("x").as_int64()), static_cast<int>(building.as_object().at("y").as_int64())},
                              {static_cast<int>(building.as_object().at("w").as_int64()), static_cast<int>(building.as_object().at("h").as_int64())}};
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
                              {static_cast<int>(office.as_object().at("x").as_int64()), static_cast<int>(office.as_object().at("y").as_int64())}, 
                              {static_cast<int>(office.as_object().at("offsetX").as_int64()), static_cast<int>(office.as_object().at("offsetY").as_int64())}};
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
            if (pair.key() == "buildings") {
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
    std::string json_as_string = LoadJsonFileAsString(json_path);
    boost::json::value parsed_json = boost::json::parse(json_as_string);

    model::Game game;

    AddMapsToGame(parsed_json.as_object().at("maps").as_array(), game);

    return game;
}

}  // namespace json_loader
