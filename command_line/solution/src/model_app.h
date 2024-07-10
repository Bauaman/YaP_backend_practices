#pragma once

#include <cassert>
#include <iomanip>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>

#include "types.h"
//#include "tagged.h"

namespace model {

namespace detail {
struct TokenTag {};
}  // namespace detail

using namespace std::literals;

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class Player;
class GameSession;

Token GetToken();

class Dog {
public:
    explicit Dog(Token pl_tok) :
        dog_id_(++dog_id_counter_),
        token_(pl_tok),
        dog_position_(0., 0.),
        dog_speed_(0., 0.),
        dir_("U") {
        }

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

    void SetSpeed(/*const ParamPairDouble& speed*/) {
        if (dir_ == "U") {
            dog_speed_ = {0., -default_dog_speed_};
        } else if (dir_ == "R") {
            dog_speed_ = {default_dog_speed_, 0.};
        } else if (dir_ == "D") {
            dog_speed_ = {0., default_dog_speed_};
        } else if (dir_ == "L") {
            dog_speed_ = {-default_dog_speed_, 0.};
        } else if (dir_ == "") {
            dog_speed_ = {0., 0.};
        }
    }

    void ResetSpeed() {
        dog_speed_ = {0., 0.};
    }

    void SetDefaultSpeed(double default_dog_speed) {
        default_dog_speed_ = default_dog_speed;
        //SetSpeed();
    }

    const std::string& GetDogDirection() const {
        return dir_;
    }

    void SetDogDirection(const std::string& dir) {
        if (dir != "U" && dir != "R" && dir != "L" && dir != "D" && dir != "") {
            throw std::invalid_argument("Unknown direction...");
        }
        dir_ = dir;
        SetSpeed();
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
    double default_dog_speed_ = 1.;
};

class Player {
public:
    Player(const Token& token, const std::string& name, std::shared_ptr<GameSession> sess/*, model::ParamPairDouble& dsp*/) :
        player_token_(token), 
        player_name_(name),
        session_{sess},
        player_id_(++player_id_counter_){
            SetDog(std::make_shared<Dog>(token));
            //dog_->SetPosition(dsp);
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
    /*Player**/std::shared_ptr<Player> FindPlayer(const Token& token) {
        if (players_.contains(token)) {
            return players_.at(token);
        }
        return nullptr;
    }

    /*Player&*/std::shared_ptr<Player> AddPlayer(const std::string& name, std::shared_ptr<GameSession> session) {
        Token token = GetToken();
        auto p = players_.emplace(token, std::make_shared<Player>(token, name, session));
        if (p.second) {
            return players_.at(token);
        }
        throw std::runtime_error("Failed to add player...");
    }

    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetPlayersList() const {
        return players_;
    }
    
    auto begin() {
        return players_.begin();
    }

    auto end() {
        return players_.end();
    }

private:
    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> players_;
};

}