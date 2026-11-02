#include "model.hpp"
#include "../../third_party/nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace hellboost::browser {
namespace {
using Json = nlohmann::json;

std::string lower_ascii(std::string text) {
    for (auto& ch : text) if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + ('a' - 'A'));
    return text;
}

bool alphanumeric(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
}

std::string text_field(const Json& item, const char* key, bool nullable = false) {
    const auto field = item.find(key);
    if (field == item.end() || (nullable && field->is_null())) return {};
    if (!field->is_string()) throw std::runtime_error(std::string("Camp text invalid: ") + key);
    auto text = field->get<std::string>();
    if (text.size() > 512) throw std::runtime_error("Camp text prea lung in API");
    const auto control = [](char ch) {
        return static_cast<unsigned char>(ch) < 32 || ch == 127;
    };
    if (std::string(key) == "address" && std::any_of(text.begin(), text.end(), control))
        throw std::runtime_error("Caracter de control in adresa API");
    text.erase(std::remove_if(text.begin(), text.end(), control), text.end());
    return text;
}

int integer_field(const Json& item, const char* key, int fallback, int maximum) {
    const auto field = item.find(key);
    if (field == item.end()) return fallback;
    if (!field->is_number_integer()) throw std::runtime_error(std::string("Camp numeric invalid: ") + key);
    if (field->is_number_unsigned()) {
        const auto value = field->get<uint64_t>();
        if (value > static_cast<uint64_t>(maximum)) throw std::runtime_error("Valoare API in afara limitelor");
        return static_cast<int>(value);
    }
    const auto value = field->get<int64_t>();
    if (value < 0 || value > maximum) throw std::runtime_error("Valoare API in afara limitelor");
    return static_cast<int>(value);
}
}

bool valid_address(const std::string& address) {
    const auto colon = address.find(':');
    if (colon == std::string::npos || colon == 0 || colon > 253 || address.size() > 255 ||
        !alphanumeric(address.front()) || !alphanumeric(address[colon - 1])) return false;
    for (std::size_t i = 0; i < colon; ++i) {
        const char ch = address[i];
        if (!alphanumeric(ch) && ch != '.' && ch != '-') return false;
    }
    const auto digits = address.size() - colon - 1;
    if (digits == 0 || digits > 5) return false;
    unsigned port = 0;
    for (std::size_t i = colon + 1; i < address.size(); ++i) {
        if (address[i] < '0' || address[i] > '9') return false;
        port = port * 10 + static_cast<unsigned>(address[i] - '0');
    }
    return port >= 1 && port <= 65535;
}

LaunchMode select_launch_mode(bool prefer_steam, bool steam_available) {
    return prefer_steam && steam_available ? LaunchMode::steam : LaunchMode::local;
}

bool valid_nickname(const std::string& nickname) {
    if (nickname.empty() || nickname.size() > 128) return false;
    unsigned characters = 0;
    for (std::size_t i = 0; i < nickname.size();) {
        const auto ch = static_cast<unsigned char>(nickname[i++]);
        ++characters;
        if (ch < 128) {
            if (!alphanumeric(static_cast<char>(ch)) && ch != ' ' && ch != '_' && ch != '-' && ch != '.') return false;
            continue;
        }
        unsigned count = 0, value = 0, minimum = 0;
        if (ch >= 0xc2 && ch <= 0xdf) { count = 1; value = ch & 0x1f; minimum = 0x80; }
        else if (ch >= 0xe0 && ch <= 0xef) { count = 2; value = ch & 0x0f; minimum = 0x800; }
        else if (ch >= 0xf0 && ch <= 0xf4) { count = 3; value = ch & 7; minimum = 0x10000; }
        else return false;
        if (count > nickname.size() - i) return false;
        for (unsigned j = 0; j < count; ++j) {
            const auto next = static_cast<unsigned char>(nickname[i++]);
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff) || (value >= 0x80 && value <= 0x9f)) return false;
    }
    return characters <= 32;
}

std::vector<Server> parse_servers(const std::string& body) {
    if (body.size() > max_response_bytes) throw std::runtime_error("Raspuns API prea mare");
    std::vector<std::set<std::string>> keys;
    std::size_t nodes = 0;
    const auto callback = [&keys, &nodes](int depth, Json::parse_event_t event, Json& parsed) {
        if (depth > 16 || ++nodes > 300000) throw std::runtime_error("Structura JSON depaseste limitele");
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        if (event == Json::parse_event_t::object_end) keys.pop_back();
        if (event == Json::parse_event_t::key && !keys.back().insert(parsed.get<std::string>()).second)
            throw std::runtime_error("Camp JSON duplicat");
        return true;
    };
    Json document;
    try { document = Json::parse(body, callback); }
    catch (const Json::exception&) { throw std::runtime_error("API-ul nu a returnat JSON valid"); }
    if (!document.is_object() || !document.contains("servers") || !document["servers"].is_array())
        throw std::runtime_error("Lipseste lista servers din API");
    if (document["servers"].size() > max_servers) throw std::runtime_error("Prea multe servere in API");
    std::vector<Server> result;
    std::set<std::string> seen;
    for (const auto& item : document["servers"]) {
        if (!item.is_object()) throw std::runtime_error("Inregistrare de server invalida");
        if (item.contains("online")) {
            if (!item["online"].is_boolean()) throw std::runtime_error("Stare online invalida");
            if (!item["online"].get<bool>()) continue;
        }
        Server server;
        server.address = text_field(item, "address");
        if (!valid_address(server.address)) throw std::runtime_error("Adresa serverului din API este invalida");
        if (!seen.insert(lower_ascii(server.address)).second) continue;
        server.name = text_field(item, "hostname");
        if (server.name.empty()) server.name = server.address;
        server.map = text_field(item, "map");
        server.promo = text_field(item, "promo", true);
        server.country = text_field(item, "country");
        server.rank = integer_field(item, "rank", static_cast<int>(result.size() + 1), std::numeric_limits<int>::max());
        if (server.rank == 0) throw std::runtime_error("Pozitie API invalida");
        server.players = integer_field(item, "players", 0, 255);
        server.max_players = integer_field(item, "max_players", 0, 255);
        result.push_back(std::move(server));
    }
    return result;
}

std::vector<Server> saved_servers(const std::vector<std::string>& addresses, const std::vector<Server>& online) {
    std::map<std::string, const Server*> by_address;
    for (const auto& server : online) by_address.emplace(lower_ascii(server.address), &server);
    std::vector<Server> result;
    std::set<std::string> seen;
    for (const auto& address : addresses) {
        if (!valid_address(address) || !seen.insert(lower_ascii(address)).second) continue;
        const auto found = by_address.find(lower_ascii(address));
        if (found != by_address.end()) result.push_back(*found->second);
        else {
            Server server;
            server.address = server.name = address;
            server.known = false;
            result.push_back(std::move(server));
        }
    }
    return result;
}

std::vector<Server> filter_servers(const std::vector<Server>& servers, const std::string& query) {
    if (query.empty()) return servers;
    const auto needle = lower_ascii(query);
    std::vector<Server> result;
    for (const auto& server : servers) {
        if (lower_ascii(server.name + " " + server.address + " " + server.map).find(needle) != std::string::npos)
            result.push_back(server);
    }
    return result;
}

std::vector<std::string> game_arguments(LaunchMode mode, const std::string& nickname, bool windowed, const std::string& address) {
    if (!valid_nickname(nickname)) throw std::runtime_error("Nume invalid: foloseste 1-32 litere, cifre, spatii sau . - _");
    if (!address.empty() && !valid_address(address)) throw std::runtime_error("Server invalid: foloseste domeniu:port sau IP:port");
    std::vector<std::string> args;
    if (mode == LaunchMode::steam) args = {"-applaunch", "10"};
    args.insert(args.end(), {"-game", "cstrike", windowed ? "-windowed" : "-fullscreen", "+name", nickname});
    if (!address.empty()) args.insert(args.end(), {"+connect", address});
    return args;
}

std::wstring quote_argument(const std::wstring& argument) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const auto ch : argument) {
        if (ch == L'\\') { ++slashes; continue; }
        if (ch == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
        } else result.append(slashes, L'\\');
        slashes = 0;
        result += ch;
    }
    result.append(slashes * 2, L'\\');
    result += L'\"';
    return result;
}
} // namespace hellboost::browser
