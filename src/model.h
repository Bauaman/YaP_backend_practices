#pragma once

#include "tagged.h"

#include <cassert>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace model {

/*
namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;
*/
using namespace std::literals;

using Dimention = int;
using Coord = Dimention;

//class GameSession;

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
        end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept :
        start_{start},
        end_{start.x, end_y} {}

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

private:
    Point start_;
    Point end_;
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

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    Id id_;
    std::string name_;

    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

/*
class Dog {
public:
    explicit Dog(Token pl_tok) :
        dog_id_(++dog_id_counter_),
        player_token_(pl_tok) {}

    int GetId() const noexcept {
        return dog_id_;
    }

    const std::string& GetDogName() const noexcept {
        return dog_name_;
    }

    void SetDogName(std::string dog_name) {
        dog_name_ = dog_name;
    }

    Token GetToken() const {
        return player_token_;
    }

    void SetToken(Token pl_token) {
        player_token_ = pl_token;
    }

private:
    int dog_id_;
    std::string dog_name_;
    Token player_token_;
    static int dog_id_counter_;
};
*/
/*
class Player {
public:
    Player(const Token& token, const std::string& name, GameSession& session, Dog& dog) :
        player_token_(token), 
        player_name_(name),
        session_(session),
        dog_(dog),
        player_id_(++player_id_counter_){}

    Token GetPlayerToken() const {
        return player_token_;
    }

    int GetId() const noexcept {
        return player_id_;
    }

    const GameSession& GetPlayersSession() const noexcept {
        return session_;
    }

    std::string GetName() const {
        return std::string(player_name_);
    }

private:
    Token player_token_;
    std::string player_name_;
    GameSession& session_;
    Dog& dog_;
    int player_id_;
    static int player_id_counter_;
};
*/
/*
class PlayerToken {
public:
    Token GetToken() {
        std::stringstream ss;
        ss << std::setw(16) << std::setfill('0') << std::hex << generator1_(); 
        ss << std::setw(16) << std::setfill('0') << std::hex << generator2_(); 
        std::string result = ss.str();
        assert(result.size() == 32);
        return Token(ss.str());
    }

    const Player* FindPlayer(const Token& token) const noexcept {
        auto it = player_indexes_.find(token);
        if (it != player_indexes_.end()) {
            return &players_.at(it->second);
        }
        return nullptr;
    }

    Player& AddPlayer(Player player);

    const std::vector<Player>& GetPlayers() const noexcept {
        return players_;
    }

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным

    std::vector<Player> players_;
    std::unordered_map<Token, size_t, TokenHasher> player_indexes_;
};
*/
/*
class GameSession {
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

public:
    explicit GameSession(const Map& map) :
        map_(map) {}

    void AddDog(Dog dog) {
        dogs_.push_back(dog);
    }

    const std::vector<Dog>& GetDogs() const {
        return dogs_;
    }

private:
    const Map& map_;
    std::vector<Dog> dogs_;
};
*/
/*
class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    
    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession& GetGameSession(const Map::Id& id) {
        if (game_sessions_.contains(id)) {
            return game_sessions_.at(id);
        }

        const Map* map = FindMap(id);
        if (!map) {
            throw std::invalid_argument("Map "s + *id + " doesn't exist"s);
        }
        auto pair = game_sessions_.emplace(id, *map);
        if (!pair.second) {
            throw std::logic_error("Failed to emplace map "s + *id);
        }
        return pair.first->second;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::unordered_map<Map::Id, GameSession, util::TaggedHasher<Map::Id>> game_sessions_;

};

*/

} //namespace model