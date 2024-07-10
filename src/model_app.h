#pragma once

#include <cassert>
#include <iomanip>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>
#include "tagged.h"

namespace model {

namespace detail {
struct TokenTag {};
}  // namespace detail

using namespace std::literals;

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

template <typename T>
struct ParamPair {
    ParamPair() = default;

    ParamPair(const T& x, const T& y) :
        x_(x),
        y_(y) {}

    T x_;
    T y_;
};

using ParamPairInt = ParamPair<int>;
using ParamPairDouble = ParamPair<double>;

class Player;
class GameSession;

Token GetToken();

class Dog {
public:
    explicit Dog(Token pl_tok) :
        dog_id_(++dog_id_counter_),
        token_(pl_tok),
        dog_position_(0., 0.),
        dog_speed_(0., 0.) {
            SetDogDirection("U");}

    int GetId() const {
        return dog_id_;
    }

    Token GetToken() const {
        return token_;
    }

    bool SetToken(const Token& pl_token) {
        if ((*token_).empty()) {
            token_ = pl_token;
            return true;
        }
        return false;
    }

    const ParamPairDouble GetDogPosition() const {
        return dog_position_;
    }

    const ParamPairDouble GetDogSpeed() const {
        return dog_speed_;
    }

    void SetPosition(const ParamPairDouble& position) {
        dog_position_.x_ = position.x_;
        dog_position_.y_ = position.y_;
    }

    void SetSpeed(const ParamPairDouble& speed) {
        dog_speed_.x_ = speed.x_;
        dog_speed_.y_ = speed.y_;
    }

    const std::string& GetDogDirection() const {
        return dir_;
    }

    void SetDogDirection(const std::string& dir) {
        if (dir == "U" || dir == "R" || dir == "D" || dir == "L") {
            dir_ = dir;
            return;
        }
        throw std::invalid_argument("Unknown direction...");
    }

    void Move(const ParamPairDouble& move_dist) {
        dog_position_.x_ += move_dist.x_;
        dog_position_.y_ += move_dist.y_;
    }

private:
    Token token_;
    int dog_id_;
    static int dog_id_counter_;

    std::string dir_;
    ParamPairDouble dog_position_;
    ParamPairDouble dog_speed_;
};

class Player {
public:
    Player(const Token& token, const std::string& name) :
        player_token_(token), 
        player_name_(name),
        player_id_(++player_id_counter_){
            SetDog(std::make_shared<Dog>(token));
        }

    Token GetPlayerToken() const {
        return player_token_;
    }

    int GetId() const noexcept {
        return player_id_;
    }

    const std::shared_ptr<GameSession> GetPlayersSession() const {
        return session_;
    }

    std::shared_ptr<GameSession> GetPlayersSession() {
        return session_;
    }

    const std::string& GetName() const {
        return player_name_;
    }

    void SetName(const std::string& name) {
        if (!player_name_.empty()) {
            throw std::logic_error("Cannot rename player...");
        }
        if (name.empty()) {
            throw std::logic_error("Cannot set empty name...");
        }
        player_name_ = name;
    }

    std::shared_ptr<Dog> GetDog() const {
        return dog_;
    }

    void SetDog(std::shared_ptr<Dog> dog) {
        if (dog_) {
            throw std::logic_error("Player already owns a dog...");
        }
        if (!dog) {
            throw std::logic_error("Dog not specified...");
        }
        dog_ = dog;
    }

private:
    Token player_token_;
    std::string player_name_;
    std::shared_ptr<GameSession> session_;
    std::shared_ptr<Dog> dog_;
    int player_id_;
    static int player_id_counter_;
};

class PlayerList {
public:    
    Player* FindPlayer(const Token& token) {
        if (players_.contains(token)) {
            return &players_.at(token);
        }
        return nullptr;
    }

    Player& AddPlayer(const std::string& name) {
        Token token = GetToken();
        auto p = players_.emplace(token, Player(token, name));
        if (p.second) {
            return players_.at(token);
        }
        throw std::runtime_error("Failed to add player...");
    }

    const std::unordered_map<Token, Player, TokenHasher>& GetPlayersList() const {
        return players_;
    }
    
    auto begin() {
        return players_.begin();
    }

    auto end() {
        return players_.end();
    }

private:
    std::unordered_map<Token, Player, TokenHasher> players_;
};

}