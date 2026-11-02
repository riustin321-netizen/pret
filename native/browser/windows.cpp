#include "windows.hpp"
#include "../../third_party/nlohmann/json.hpp"

#include <commdlg.h>
#include <winhttp.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace hellboost::browser {
namespace {
constexpr wchar_t settings_key[] = L"Software\\HellBoost";

bool steam_preference(bool fallback) {
    DWORD value = fallback ? 1 : 0, bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, settings_key, L"PreferSteam", RRF_RT_REG_DWORD, nullptr, &value, &bytes) != ERROR_SUCCESS) return fallback;
    return value != 0;
}

std::runtime_error windows_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " (Windows " + std::to_string(GetLastError()) + ")");
}

std::wstring registry_text(HKEY hive, const wchar_t* key, const wchar_t* name, REGSAM view = 0) {
    HKEY handle = nullptr;
    if (RegOpenKeyExW(hive, key, 0, KEY_QUERY_VALUE | view, &handle) != ERROR_SUCCESS) return {};
    DWORD type = 0, bytes = 0;
    auto status = RegQueryValueExW(handle, name, nullptr, &type, nullptr, &bytes);
    std::vector<wchar_t> buffer;
    if (status == ERROR_SUCCESS && type == REG_SZ && bytes <= 65536 && bytes % sizeof(wchar_t) == 0) {
        buffer.resize(bytes / sizeof(wchar_t) + 1, 0);
        status = RegQueryValueExW(handle, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer.data()), &bytes);
    } else status = ERROR_INVALID_DATA;
    RegCloseKey(handle);
    if (status != ERROR_SUCCESS || type != REG_SZ || buffer.empty()) return {};
    return std::wstring(buffer.data());
}

bool regular_file(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

struct InternetHandle {
    HINTERNET value = nullptr;
    explicit InternetHandle(HINTERNET handle) : value(handle) {
        if (!value) throw windows_error("WinHTTP");
    }
    ~InternetHandle() { WinHttpCloseHandle(value); }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};

struct PlatformSession : InternetHandle {
    PlatformSession() : InternetHandle(WinHttpOpen(L"HellBoost/0.3", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)) {
        if (!WinHttpSetTimeouts(value, 5000, 5000, 5000, 5000)) throw windows_error("Timeout WinHTTP");
        DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
        if (!WinHttpSetOption(value, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols))) throw windows_error("TLS 1.2 indisponibil");
    }
};

struct PlatformRequest {
    PlatformSession session;
    InternetHandle connection;
    InternetHandle request;

    PlatformRequest() :
        connection(WinHttpConnect(session.value, L"hellboost.net", INTERNET_DEFAULT_HTTPS_PORT, 0)),
        request(WinHttpOpenRequest(connection.value, L"GET", L"/api.php", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)) {
        DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (!WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect))) throw windows_error("Politica redirect HTTPS");
    }
};
}

std::wstring wide(const std::string& text) {
    if (text.empty()) return {};
    const auto length = static_cast<int>(text.size());
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, nullptr, 0);
    if (!size) throw std::runtime_error("Text UTF-8 invalid");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, result.data(), size);
    return result;
}

std::string utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const auto length = static_cast<int>(text.size());
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("Text Unicode invalid");
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length, result.data(), size, nullptr, nullptr);
    return result;
}

Settings load_settings() {
    Settings settings;
    settings.prefer_steam = steam_preference(true);
    try {
        const auto stored = registry_text(HKEY_CURRENT_USER, settings_key, L"SettingsJson");
        if (stored.empty()) return settings;
        const auto document = nlohmann::json::parse(utf8(stored));
        if (!document.is_object() || document.value("version", 0) != 1) return settings;
        const auto nickname = document.value("nickname", std::string("Player"));
        if (valid_nickname(nickname)) settings.nickname = wide(nickname);
        settings.windowed = document.value("windowed", true);
        settings.prefer_steam = steam_preference(document.value("prefer_steam", true));
        settings.local_game = wide(document.value("local_game", std::string{}));
        for (const auto* key : {"favorites", "history"}) {
            const auto items = document.find(key);
            auto& output = std::string(key) == "favorites" ? settings.favorites : settings.history;
            if (items == document.end() || !items->is_array()) continue;
            for (const auto& item : *items) {
                if (output.size() >= 100) break;
                if (!item.is_string()) continue;
                auto address = item.get<std::string>();
                if (valid_address(address) && std::find(output.begin(), output.end(), address) == output.end()) output.push_back(std::move(address));
            }
        }
    } catch (const std::exception&) {
        Settings fallback;
        fallback.prefer_steam = steam_preference(true);
        return fallback;
    }
    return settings;
}

void save_settings(const Settings& settings, bool save_steam_preference) {
    const nlohmann::json document = {
        {"version", 1}, {"nickname", utf8(settings.nickname)}, {"windowed", settings.windowed}, {"prefer_steam", settings.prefer_steam},
        {"local_game", utf8(settings.local_game.wstring())}, {"favorites", settings.favorites}, {"history", settings.history}
    };
    const auto text = wide(document.dump());
    if (text.size() > 32760) throw std::runtime_error("Prea multe setari salvate");
    HKEY handle = nullptr;
    const auto opened = RegCreateKeyExW(HKEY_CURRENT_USER, settings_key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &handle, nullptr);
    if (opened != ERROR_SUCCESS) throw std::runtime_error("Nu pot salva setarile in profilul Windows");
    const auto written = RegSetValueExW(handle, L"SettingsJson", 0, REG_SZ, reinterpret_cast<const BYTE*>(text.c_str()), static_cast<DWORD>((text.size() + 1) * sizeof(wchar_t)));
    DWORD preference = settings.prefer_steam ? 1 : 0;
    const auto mode_written = written == ERROR_SUCCESS && save_steam_preference ? RegSetValueExW(handle, L"PreferSteam", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&preference), sizeof(preference)) : written;
    RegCloseKey(handle);
    if (written != ERROR_SUCCESS || mode_written != ERROR_SUCCESS) throw std::runtime_error("Salvarea setarilor a esuat");
}

std::filesystem::path application_directory() {
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) throw windows_error("Locatia launcherului");
    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

std::filesystem::path find_steam() {
    std::vector<std::filesystem::path> candidates;
    auto path = registry_text(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamExe");
    if (!path.empty()) candidates.emplace_back(path);
    path = registry_text(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath", KEY_WOW64_32KEY);
    if (!path.empty()) candidates.push_back(std::filesystem::path(path) / L"steam.exe");
    for (const auto& candidate : candidates) {
        if (_wcsicmp(candidate.filename().c_str(), L"steam.exe") == 0 && regular_file(candidate)) return candidate;
    }
    return {};
}

bool valid_local_game(const std::filesystem::path& path) {
    std::error_code error;
    return !path.empty() && _wcsicmp(path.filename().c_str(), L"hl.exe") == 0 && regular_file(path) &&
        std::filesystem::is_directory(path.parent_path() / L"cstrike", error);
}

std::filesystem::path find_local_game(const Settings& settings) {
    const auto directory = application_directory();
    for (const auto& candidate : {directory / L"hl.exe", directory / L"client" / L"hl.exe", settings.local_game})
        if (valid_local_game(candidate)) return candidate;
    return {};
}

std::filesystem::path choose_local_game(HWND parent) {
    std::wstring buffer(32768, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = parent;
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrFilter = L"Half-Life (hl.exe)\0hl.exe\0\0";
    dialog.lpstrTitle = L"Alege hl.exe din instalarea ta CS 1.6";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) {
        if (CommDlgExtendedError()) throw std::runtime_error("Dialogul de selectie nu a putut fi deschis");
        return {};
    }
    buffer.resize(wcslen(buffer.c_str()));
    std::filesystem::path path(buffer);
    if (!valid_local_game(path)) throw std::runtime_error("Selecteaza hl.exe dintr-un folder care contine cstrike");
    return path;
}

std::wstring start_game(const Settings& settings, const std::string& address) {
    const auto steam = settings.prefer_steam ? find_steam() : std::filesystem::path{};
    const auto mode = select_launch_mode(settings.prefer_steam, !steam.empty());
    const auto executable = steam.empty() ? find_local_game(settings) : steam;
    if (executable.empty()) throw std::runtime_error("Steam nu a fost gasit. Alege instalarea locala hl.exe.");
    const auto arguments = game_arguments(mode, utf8(settings.nickname), settings.windowed, address);
    auto command = quote_argument(executable.wstring());
    for (const auto& argument : arguments) command += L" " + quote_argument(wide(argument));
    if (command.size() >= 32767) throw std::runtime_error("Comanda de pornire este prea lunga");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const auto directory = executable.parent_path().wstring();
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, directory.c_str(), &startup, &process))
        throw windows_error("Pornirea jocului a esuat");
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return mode == LaunchMode::steam ? L"Steam" : L"local / non-Steam";
}

void verify_platform_configuration() {
    PlatformRequest handles;
    (void)handles;
}

std::vector<Server> fetch_platform(const std::atomic_bool& cancelled) {
    PlatformRequest handles;
    auto request = handles.request.value;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    const auto check = [&] {
        if (cancelled.load()) throw std::runtime_error("Cerere anulata");
        if (std::chrono::steady_clock::now() > deadline) throw std::runtime_error("API-ul HellBoost a depasit timpul de raspuns");
    };
    check();
    if (!WinHttpSendRequest(request, L"Accept: application/json\r\n", static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request, nullptr))
        throw windows_error("Conexiunea HTTPS catre hellboost.net a esuat");
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX))
        throw windows_error("Status HTTP invalid");
    if (status != 200) throw std::runtime_error("HellBoost API a raspuns HTTP " + std::to_string(status) + ". Verifica https://hellboost.net/api.php");
    std::string body;
    char buffer[8192];
    while (true) {
        check();
        DWORD received = 0;
        if (!WinHttpReadData(request, buffer, sizeof(buffer), &received)) throw windows_error("Citirea API a esuat");
        if (!received) break;
        if (body.size() + received > max_response_bytes) throw std::runtime_error("Raspuns API prea mare");
        body.append(buffer, received);
    }
    check();
    return parse_servers(body);
}
} // namespace hellboost::browser
