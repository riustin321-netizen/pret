#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "model.hpp"
#include <atomic>
#include <filesystem>

namespace hellboost::browser {
struct Settings {
    std::wstring nickname = L"Player";
    bool windowed = true;
    std::filesystem::path local_game;
    std::vector<std::string> favorites;
    std::vector<std::string> history;
};

std::wstring wide(const std::string& text);
std::string utf8(const std::wstring& text);
Settings load_settings();
void save_settings(const Settings& settings);
std::filesystem::path application_directory();
std::filesystem::path find_steam();
std::filesystem::path find_local_game(const Settings& settings);
bool valid_local_game(const std::filesystem::path& path);
std::filesystem::path choose_local_game(HWND parent);
std::wstring start_game(const Settings& settings, const std::string& address);
// Validate WinHTTP handle configuration without sending a network request.
void verify_platform_configuration();
std::vector<Server> fetch_platform(const std::atomic_bool& cancelled);
} // namespace hellboost::browser
