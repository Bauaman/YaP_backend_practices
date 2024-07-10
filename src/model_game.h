#pragma once

#include "model_app.h"
#include "model.h"

namespace model {

class GameSession {
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

public:
    explicit GameSession(const Map::Id& map) :
        map_(map) {}

    const Map::Id& GetMapId() const {
        return map_;
    }

    void AddDog(std::shared_ptr<Dog> dog) {
        dogs_.emplace_back(dog);
    }

    const std::vector<std::shared_ptr<Dog>> GetDogs() {
        std::vector<std::shared_ptr<Dog>> result;
        result.reserve(dogs_.size());

        auto iter = dogs_.begin();
        while (iter != dogs_.end()) {
            if (iter->expired()) {
                iter = dogs_.erase(iter);
            } else {
                result.push_back(iter->lock());
                ++iter;
            }
        }
        return result;

    }

    const std::shared_ptr<Dog> GetDog(const Token& token) const {
        for (const auto& weak_dog : dogs_) {
            if (auto dog = weak_dog.lock()) {
                if (dog->GetToken() == token) {
                    return dog;
                }
            }
        }
        return nullptr;
    }

private:
    const Map::Id map_;
    std::vector<std::weak_ptr<Dog>> dogs_;
};

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

    std::shared_ptr<GameSession> GetGameSession(const Map::Id& id) {
        const Map* map = FindMap(id);
        if (!map) {
            throw std::invalid_argument("Map "s + *id + " doesn't exist"s);
        }
        for (auto& session : game_sessions_) {
            if (session->GetMapId() == id) {
                return session;
            }
        }

        auto game_session = std::make_shared<GameSession>(id);
        game_sessions_.push_back(game_session);

        return game_session;
    }

    void SetDefaultDogSpeed(ParamPairDouble speed) {
        default_dog_speed_ = speed;
    }

    ParamPairDouble SetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<std::shared_ptr<GameSession>> game_sessions_;

    ParamPairDouble default_dog_speed_ = ParamPairDouble{1., 1.};

};

}