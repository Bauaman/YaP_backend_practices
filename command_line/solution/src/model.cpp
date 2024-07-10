#include "model.h"

namespace model {
using namespace std::literals;

double RoundToTwoDigits(double d) {
    return ((int)(d * 100 + 0.5) / 100.0);
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    std::shared_ptr<Road> road_ptr = std::make_shared<Road>(roads_.back());
    //std::cout << "road emplaced: " << road_ptr << std::endl;
    
    if (road.IsHorizontal()) {
        //std::cout << "Adding Horizontal road " << std::endl;
        for (Coord x = std::min(road_ptr->GetStart().x, road_ptr->GetEnd().x); x <= std::max(road_ptr->GetStart().x, road_ptr->GetEnd().x); x++) {
            //std::cout << "adding point: {" << x << ", " << road_ptr->GetStart().y << "} " << road_ptr << std::endl; 
            coord_to_road[{x, road_ptr->GetStart().y}].emplace(road_ptr);
        }
    }
    if (road.IsVertical()) {
        //std::cout << "Adding Vertical road " << std::endl;
        for (Coord y = std::min(road_ptr->GetStart().y, road_ptr->GetEnd().y); y <= std::max(road_ptr->GetStart().y, road_ptr->GetEnd().y); y++) {
            //std::cout << "adding point: {" << road_ptr->GetStart().x << ", " << y << "} " << road_ptr << std::endl; 
            coord_to_road[{road_ptr->GetStart().x, y}].emplace(road_ptr);
        }        
    }
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
        try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

std::set<std::shared_ptr<Road>> Map::GetRoadsByCoords(Point p) const {
    if (coord_to_road.contains(p)) {
        return coord_to_road.at(p);
    }
    return {};    
}

ParamPairDouble Map::GetRandomDogPosition() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads to put dog on...");
    }

    std::random_device rd;
    std::mt19937 gen{rd()};
    int road_number = 0;
    if (roads_.size() > 1) {
        std::uniform_int_distribution<int> dist(0, roads_.size()-1);
        road_number = dist(gen);
    }
    Road road = roads_.at(road_number);
    Point road_start = road.GetStart();
    Point road_end = road.GetEnd();

    ParamPairDouble random_point{static_cast<double>(road_start.x), static_cast<double>(road_start.y)};
    double width;
    {
        std::uniform_real_distribution<> dist(-0.4, 0.4);
        width = dist(gen);
        width = RoundToTwoDigits(width);
    }

    if (road.IsHorizontal()) {
        std::uniform_real_distribution<> dist(road_start.x, road_end.x);
        random_point.x_ = dist(gen);
        random_point.y_ += width;
    }
    if (road.IsVertical()) {
        std::uniform_real_distribution<> dist(road_start.y, road_end.y);
        random_point.y_ = dist(gen);
        random_point.x_ += width;
    }
    random_point.x_ = RoundToTwoDigits(random_point.x_); 
    random_point.y_ = RoundToTwoDigits(random_point.y_);

    return random_point;
}

ParamPairDouble Map::GetStartPosition(bool random) const {
    if (random) {
        return GetRandomDogPosition();
    }
    Point p = roads_.at(0).GetStart();
    return {p.x*1., p.y*1.};
}

void Map::PrintCoordToRoad() const {
    for (auto [point, road] : coord_to_road) {
        
        std::cout << "{" << point.x << ", " << point.y << "} " << ": " << std::endl;
        for (auto r : road) {
            std::cout << "road ptr " << r << std::endl;
        }
    }
}

}