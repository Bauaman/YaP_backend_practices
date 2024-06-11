#pragma once


#include <filesystem>
#include <iostream>
#include <fstream>
#include "model.h"

namespace json_loader {

std::string LoadJsonFileAsString(const std::filesystem::path& json_path);
model::Game LoadGame(const std::filesystem::path& json_path);

void SetKeySequenceRoad(const boost::json::value& road, model::Road& road_);
void AddMapsToGame (const boost::json::value& parsed, model::Game& game);
void AddOfficesToMap(const boost::json::value& parsed, model::Map& map);
void AddBuildingsToMap(const boost::json::value& parsed, model::Map& map);
void AddRoadsToMap(const boost::json::value& parsed, model::Map& map);

}  // namespace json_loader
