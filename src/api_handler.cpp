#include "api_handler.h"

namespace http_handler {

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

}