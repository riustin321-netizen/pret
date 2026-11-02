#pragma once

#include <string>
#include <vector>

namespace hellboost::browser {
constexpr std::size_t max_response_bytes = 2 * 1024 * 1024;
constexpr std::size_t max_servers = 5000;

struct Server {
    int rank = 0;
    std::string address;
    std::string name;
    std::string map;
    std::string promo;
    std::string country;
    int players = 0;
    int max_players = 0;
    bool known = true;
};

enum class LaunchMode { steam, local };

bool valid_address(const std::string& address);
bool valid_nickname(const std::string& nickname);
std::vector<Server> parse_servers(const std::string& body);
std::vector<Server> saved_servers(const std::vector<std::string>& addresses, const std::vector<Server>& online);
std::vector<Server> filter_servers(const std::vector<Server>& servers, const std::string& query);
std::vector<std::string> game_arguments(LaunchMode mode, const std::string& nickname, bool windowed, const std::string& address);
// Windows CRT quoting; also quotes argv[0] when used for an executable path.
std::wstring quote_argument(const std::wstring& argument);
} // namespace hellboost::browser
