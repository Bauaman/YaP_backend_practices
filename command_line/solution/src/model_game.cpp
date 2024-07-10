#include "model_game.h"

namespace model {

void GameSession::UpdateDogsPosition(const double dt) {
    const Map& map = GetMap();
    for (auto dog : GetDogs()) {
        //std::cout << "Searching for dog: " << dog->GetId() << " at map: " << map.GetName() << "..." << std::endl;
        ParamPairDouble cur_dog_pos = dog->GetDogPosition();
        //std::cout << "Found dog at coord: " << cur_dog_pos.x_ << ", " << cur_dog_pos.y_ << std::endl;
        Point p_cur_dog_pos = {static_cast<Coord>(std::round(cur_dog_pos.x_)), static_cast<Coord>(std::round(cur_dog_pos.y_))};
        //std::cout << "point for this coord {" << p_cur_dog_pos.x << ", " << p_cur_dog_pos.y << "}" << std::endl;
        std::set<std::shared_ptr<Road>> roads_at_point = map.GetRoadsByCoords(p_cur_dog_pos);
        //std::cout << "At this point there are " << roads_at_point.size() << " roads" << std::endl;
        /*
        if (!roads_at_point.empty()) {
            //std::cout << "Roads at point " << cur_dog_pos.x_ << " " << cur_dog_pos.y_ << " are: " << std::endl;
            for (auto road : roads_at_point) {
                
                std::cout << road << std::endl;
                std::cout << "{" << road->GetStart().x << ", " << road->GetStart().y << "} - {" << road->GetEnd().x << ", " << road->GetEnd().y << "}" << std::endl;
            }
        }*/
        auto dog_speed = dog->GetDogSpeed();
        //std::cout << "dog speed is {" << dog_speed.x_ << ", " << dog_speed.y_ << "}" << std::endl;
        auto new_dog_pos = cur_dog_pos + (dog_speed * dt);
        if (CheckIfMovedProperly(roads_at_point, new_dog_pos)) {
            dog->SetPosition(new_dog_pos);
        } else {
            std::cout << "Dog moved out of road" << std::endl;
            SetMaxMoveForTick(roads_at_point, new_dog_pos);
            dog->SetPosition(new_dog_pos);
            dog->ResetSpeed();
        }
    }
}

bool CheckIfMovedProperly(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos) {
    for (auto& road : roads) {
        //std::cout << "point: {" << new_pos.x_ << ", " << new_pos.y_ << "} ";
        //std::cout << "Road: x =" << road->GetRoadArea().left_bottom.x_ << "..." << road->GetRoadArea().right_top.x_;
        //std::cout << " y =" << road->GetRoadArea().left_bottom.y_ << "..." << road->GetRoadArea().right_top.y_ << std::endl;
        if (road->PointIsOnRoad(new_pos)) {
            return true;
        }
    }
    return false;
}

RoadArea CreateMaxMovingCoords(std::set<std::shared_ptr<Road>>& roads) {
    ParamPairDouble min_c = roads.begin()->get()->GetRoadArea().left_bottom;
    ParamPairDouble max_c = roads.begin()->get()->GetRoadArea().right_top;
    for (auto& road : roads) {
        if (min_c.x_ > road->GetRoadArea().left_bottom.x_) {min_c.x_ = road->GetRoadArea().left_bottom.x_;}
        if (min_c.y_ > road->GetRoadArea().left_bottom.y_) {min_c.y_ = road->GetRoadArea().left_bottom.y_;}
        if (max_c.x_ < road->GetRoadArea().right_top.x_) {max_c.x_ = road->GetRoadArea().right_top.x_;}
        if (max_c.y_ < road->GetRoadArea().right_top.y_) {max_c.y_ = road->GetRoadArea().right_top.y_;}        
    }
    return {min_c, max_c};
}

void SetMaxMoveForTick(std::set<std::shared_ptr<Road>>& roads, ParamPairDouble& new_pos) {
    RoadArea max_possible_coords = CreateMaxMovingCoords(roads);
    if (new_pos.x_ < max_possible_coords.left_bottom.x_) {new_pos.x_ = max_possible_coords.left_bottom.x_;}
    if (new_pos.y_ < max_possible_coords.left_bottom.y_) {new_pos.y_ = max_possible_coords.left_bottom.y_;}
    if (new_pos.x_ > max_possible_coords.right_top.x_) {new_pos.x_ = max_possible_coords.right_top.x_;}
    if (new_pos.y_ > max_possible_coords.right_top.y_) {new_pos.y_ = max_possible_coords.right_top.y_;}
}

void Game::AddMap(Map map) {
    //std::cout << "Point/road for this map is: " << std::endl;
    //map.PrintCoordToRoad();
    //std::cout << std::endl;
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

}