#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

struct Args {
    std::string config_file_path;
    std::string static_root;
    unsigned int tick_period = 0;
    bool random_spawn = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options:"};
    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<unsigned int>(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file_path)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::value<bool>(&args.random_spawn), "spawn dogs at random position");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (vm.contains("config-file") && vm.contains("www-root")) {
        return args;
    } else {
        throw std::runtime_error("Usage: game_server --tick-period[int, optional] --config-file <game-config-json> --www-root <dir-to-content> --randomize-spawn-points[bool, optional]");
    }
    return std::nullopt;
}