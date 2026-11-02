#include "../browser/model.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

using namespace hellboost::browser;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #condition); std::exit(1); \
} } while (false)

template <typename Function> void rejects(Function function) {
    try { function(); }
    catch (const std::runtime_error&) { return; }
    CHECK(false);
}

int main() {
    const std::string body = R"({"servers":[
        {"rank":1,"address":"top.example.com:27015","hostname":"TOP","players":1,"max_players":32,"promo":"top","country":"RO","map":"de_dust2","online":true},
        {"rank":2,"address":"busy.example.com:27015","hostname":"Busy","players":30,"max_players":32,"promo":null},
        {"rank":3,"address":"TOP.example.com:27015","hostname":"duplicate"},
        {"rank":4,"address":"offline.example.com:27015","hostname":"Offline","online":false}
    ]})";
    const auto rows = parse_servers(body);
    CHECK(rows.size() == 2);
    CHECK(rows[0].name == "TOP" && rows[0].players == 1 && rows[1].players == 30);
    CHECK(rows[1].promo.empty());
    CHECK(rows[0].country == "RO" && rows[0].map == "de_dust2");
    const auto filtered = filter_servers(rows, "DUST");
    CHECK(filtered.size() == 1 && filtered[0].address == rows[0].address);
    CHECK(filter_servers(rows, "no match").empty());
    const auto saved = saved_servers({"busy.example.com:27015", "missing.example.com:27016", "busy.example.com:27015", "bad;quit"}, rows);
    CHECK(saved.size() == 2 && saved[0].rank == 2 && !saved[1].known);
    CHECK(saved[1].name == "missing.example.com:27016");

    for (const auto* invalid : {
        "{}", "[]", "<html>error</html>", "{\"servers\":[null]}",
        "{\"servers\":[],\"servers\":[]}",
        "{\"servers\":[{\"address\":\"host:27015\",\"address\":\"other:27015\"}]}",
        "{\"servers\":[{\"address\":\"host:27015;quit\"}]}",
        "{\"servers\":[{\"address\":\"host\\n:27015\"}]}",
        "{\"servers\":[{\"address\":\"host:27015\",\"players\":true}]}",
        "{\"servers\":[{\"address\":\"host:27015\",\"players\":256}]}",
        "{\"servers\":[{\"address\":\"host:27015\",\"rank\":18446744073709551615}]}",
        "{\"servers\":[{\"address\":\"host:27015\",\"hostname\":\"\\ud800\"}]}"
    }) rejects([&] { parse_servers(invalid); });
    rejects([] { parse_servers(std::string(max_response_bytes + 1, 'x')); });
    rejects([] { parse_servers(std::string(30, '[') + "0" + std::string(30, ']')); });
    CHECK(parse_servers("{\"servers\":[]}").empty());

    CHECK(valid_address("127.0.0.1:27015"));
    CHECK(valid_address("play.example.com:65535"));
    for (const auto* invalid : {"host:0", "host:65536", "host:27015;quit", "host:27015\n", "https://host:27015", "-host:27015", "host-:27015"}) CHECK(!valid_address(invalid));
    CHECK(valid_nickname("Player One"));
    CHECK(valid_nickname("Juc\xc4\x83tor"));
    for (const auto* invalid : {"Player;quit", "Player\"", "+quit", "", "\xc0\x80", "\xed\xa0\x80"}) CHECK(!valid_nickname(invalid));
    CHECK(!valid_nickname(std::string(33, 'a')));
    const auto steam = game_arguments(LaunchMode::steam, "Player One", true, "play.example.com:27015");
    CHECK(steam[0] == "-applaunch" && steam[1] == "10");
    CHECK(steam[steam.size() - 2] == "+connect" && steam.back() == "play.example.com:27015");
    CHECK(std::find(steam.begin(), steam.end(), "Player One") != steam.end());
    const auto local = game_arguments(LaunchMode::local, "Player", false, "");
    CHECK(local.front() == "-game");
    CHECK(std::find(local.begin(), local.end(), "-fullscreen") != local.end());
    CHECK(std::find(local.begin(), local.end(), "+connect") == local.end());
    rejects([] { game_arguments(LaunchMode::local, "name;quit", true, ""); });
    CHECK(quote_argument(L"Player One") == L"\"Player One\"");
    CHECK(quote_argument(L"C:\\Games\\CS 1.6\\") == L"\"C:\\Games\\CS 1.6\\\\\"");
    CHECK(quote_argument(L"a\"b") == L"\"a\\\"b\"");
    CHECK(quote_argument(L"") == L"\"\"");
    std::puts("Browser core: platform ranking, API validation, Unicode, search, saved lists and launch commands passed.");
}
