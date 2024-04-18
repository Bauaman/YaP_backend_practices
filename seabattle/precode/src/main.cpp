#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using sock_ptr = boost::shared_ptr<tcp::socket>;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(sock_ptr socket, bool my_initiative) {
        // TODO: реализуйте самостоятельно
        std::cout << "Game started" << std::endl;  
        boost::system::error_code ec;

        do {
            std::string shot_result;
            
            if (my_initiative) {
                while (shot_result != "MISS") {
                    PrintFields();
                    std::string shot = SendMessage(socket);
                    shot_result = ReadMessage(socket, true);
                    std::pair<int, int> parsed_shot = ParseMove(std::string_view(shot)).value();
                    other_field_.Shoot(parsed_shot.second, parsed_shot.first);
                    if (shot_result == "MISS") {
                        other_field_.MarkMiss(parsed_shot.second, parsed_shot.first);
                    } else if (shot_result == "HIT") {
                        other_field_.MarkHit(parsed_shot.second, parsed_shot.first);
                    } else if (shot_result == "KILL") {
                        other_field_.MarkKill(parsed_shot.second, parsed_shot.first);
                    }
                    if (IsGameEnded()) {
                        break;
                    }
                }
                my_initiative = ! my_initiative;
            } else {
                while (shot_result != "MISS") {
                    PrintFields();
                    std::string shot = ReadMessage(socket, false);
                    std::pair<int, int> parsed_shot = ParseMove(std::string_view(shot)).value();
                    SeabattleField::ShotResult shot_res = my_field_.Shoot(parsed_shot.second, parsed_shot.first);
                    switch (shot_res)
                    {
                    case SeabattleField::ShotResult::MISS:
                        shot_result = "MISS";
                        break;
                    case SeabattleField::ShotResult::HIT:
                        shot_result = "HIT";
                        break;
                    case SeabattleField::ShotResult::KILL:
                        shot_result = "KILL";
                        break;                
                    default:
                        break;
                    }
                    SendResponce(socket, shot_result);
                    if (IsGameEnded()) {
                        break;
                    }
                
                } 
                my_initiative = ! my_initiative;
            }
        } while (!IsGameEnded());
        if (IsGameEnded()) {
            if (my_field_.IsLoser()) {
                std::cout << "You lost" << std::endl;
            } if (other_field_.IsLoser()) {
                std::cout << "You WON" << std::endl;
            }
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::string SendMessage(sock_ptr sock) {
        std::string input;
        std::cout << "Your turn: ";
        std::getline(std::cin, input);
        boost::system::error_code error;
        sock->write_some(net::buffer(input+'\n'), error);
        if (error) {
            std::cout << "Error sending message: " << error.message() << std::endl;
        }
        return input;
    }

    std::string ReadMessage(sock_ptr sock, bool turn) {
        boost::system::error_code error;
        net::streambuf stream_buf;
        if (!turn) {
            std::cout << "Waiting for turn..." << std::endl;
        }
        net::read_until(*sock, stream_buf, '\n', error);
        std::string rec_data;
        if (error) {
            std::cout << "Error reading data:" << error.message() << std::endl;
            return "error";
        }
        std::istream is(&stream_buf);
        std::getline(is, rec_data);
        if (!turn) {
            std::cout << "Shot to " << rec_data << std::endl;
        } else {
            std::cout << rec_data << std::endl;
        }
        //SendResponce(sock);
        return rec_data;
    }

    std::string SendResponce(sock_ptr sock, std::string result) {
        //std::string result;
        //std::cout << "Enter shot result: ";
        //std::getline(std::cin, result);
        boost::system::error_code error;
        sock->write_some(net::buffer(result+'\n'), error);
        if (error) {
            std::cout << "Error sending message: " << error.message() << std::endl;
        }
        return result;
    }

    // TODO: добавьте методы по вашему желанию

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..." << std::endl;

    boost::system::error_code error;
    sock_ptr sock(new tcp::socket(io_context));
    acceptor.accept(*sock, error);

    if (error) {
        std::cout << "Cannot accept connection: " << error.message() << std::endl;
        return;
    } else {
        std::cout << "Connected client: " << sock->remote_endpoint().address().to_string() << ":" << 
            sock->remote_endpoint().port() << std::endl;
        //ReadMessage(std::move(sock));
        agent.StartGame(sock, false);
    }
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    // TODO: реализуйте самостоятельно
    net::io_context io_context;
    boost::system::error_code error;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, error), port);
    if (error) {
        std::cout << "Error establishing connection: " << error.message() << std::endl;
    }
    sock_ptr sock(new tcp::socket(io_context));
    sock->connect(endpoint, error);
    if (error) {
        std::cout << "Can't connect to server" << error.message() << std::endl;
        return;
    } else {
        std::cout << "Connected to server" << std::endl;
        agent.StartGame(sock, true);
    }
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
