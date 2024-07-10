#include "model_app.h"

namespace model {

int Dog::dog_id_counter_ = 0;
int Player::player_id_counter_ = 0;

Token GetToken() {
    std::random_device random_device_;
    std::mt19937_64 generator1_{[&random_device_] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[&random_device_] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << generator1_(); 
    ss << std::setw(16) << std::setfill('0') << std::hex << generator2_(); 
    std::string result = ss.str();
    assert(result.size() == 32);
    return Token(ss.str());
}

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