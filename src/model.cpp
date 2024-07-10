#include "model.h"

namespace model {
using namespace std::literals;

//int Player::player_id_counter_ = 0;
//int Dog::dog_id_counter_ = 0;

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

/*
void Game::AddMap(Map map) {
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
*/
/*
Player& PlayerToken::AddPlayer(Player player) {
    const size_t ind = players_.size();
    if (auto [it, inserted] = player_indexes_.emplace(player.GetPlayerToken(), ind); !inserted) {
        throw std::invalid_argument("Player with token "s + *player.GetPlayerToken() + " already exists"s); 
    } else {
        try {
            return players_.emplace_back(std::move(player));
        } catch (...) {
            player_indexes_.erase(it);
            throw;
        }
    }
}
*/
}