#pragma once

#include "aux.h"
#include "types.h"
//#include "tagged.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace model {

using namespace std::literals;
/*
using Dimention = int;
using Coord = Dimention;


struct Point {
    Coord x, y;
};

struct Size {
    Dimention width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimention dx, dy;
};
*/
class Element {
public:
    void SetKeySequence(std::string str) {
        keys_.push_back(str);
    }

    std::vector<std::string> GetKeys() const {
        return keys_;
    }
private:
    std::vector<std::string> keys_;
};

class Road : public Element {
    struct  HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };
    
public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept :
        start_{start},
        end_{end_x, start.y} {
            SetRoadArea();
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept :
        start_{start},
        end_{start.x, end_y} {
            SetRoadArea();
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    bool PointIsOnRoad(ParamPairDouble& p) {
        return ((p.x_ >= road_area_.left_bottom.x_ && p.x_ <= road_area_.right_top.x_) && 
                (p.y_ >= road_area_.left_bottom.y_ && p.y_ <= road_area_.right_top.y_));

    }

    RoadArea GetRoadArea() const {
        return road_area_;
    }

private:
    Point start_;
    Point end_;

    RoadArea road_area_;

    void SetRoadArea() {
        if (this->IsHorizontal()) {
            road_area_.left_bottom = {std::min(start_.x, end_.x) - 0.4, start_.y - 0.4};
            road_area_.right_top = {std::max(start_.x, end_.x) + 0.4, start_.y + 0.4};
        }
        if (this->IsVertical()) {
            road_area_.left_bottom = {start_.x - 0.4, std::min(start_.y, end_.y) - 0.4};
            road_area_.right_top = {start_.x + 0.4, std::max(start_.y, end_.y) + 0.4};
        }
    }
};

class Building : public Element {
public:
    explicit Building(Rectangle bounds) noexcept :
        bounds_{bounds} {}

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office : public Element {
public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point pos, Offset offset) noexcept :
        id_{id},
        position_{pos},
        offset_{offset} {}

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
    std::vector<std::string> keys_;
};

class Map : public Element {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept :
        id_(std::move(id)),
        name_(std::move(name)) {}

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road);/* {
        roads_.emplace_back(road);
    }*/

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    ParamPairDouble GetRandomDogPosition() const;

    ParamPairDouble GetStartPosition(bool random) const;

    void SetMapDogSpeed(double ds) {
        //std::cout << "Map id: " << *id_ << " Setting map dog speed " << ds << std::endl;
        map_dog_speed_ = ds;
    }

    double GetMapDogSpeed() const {
        return map_dog_speed_;
    }

    std::set<std::shared_ptr<Road>> GetRoadsByCoords(Point p) const;

    void PrintCoordToRoad() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    Id id_;
    std::string name_;

    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    double map_dog_speed_;

    std::unordered_map<Point, std::set<std::shared_ptr<Road>>, PointHash> coord_to_road;
};


} //namespace model