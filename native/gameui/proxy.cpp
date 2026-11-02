#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdint>

static_assert(sizeof(void*) == 4, "GameUI007 bridge requires x86");

namespace {
using Factory = void* (__cdecl*)(const char*, int*);
using Command = void (__cdecl*)();
using AddCommand = int (__cdecl*)(char*, Command);
HMODULE module = nullptr;
HMODULE original_module = nullptr;
Factory factory = nullptr;
void* original_ui = nullptr;
const unsigned char* original_vtable = nullptr;
INIT_ONCE once = INIT_ONCE_STATIC_INIT;
HWND options_window = nullptr;
constexpr wchar_t key[] = L"Software\\HellBoost";
constexpr int checkbox_id = 201;

bool read_steam() {
    DWORD value = 1, bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, key, L"PreferSteam", RRF_RT_REG_DWORD, nullptr, &value, &bytes) != ERROR_SUCCESS) return true;
    return value != 0;
}

bool write_steam(bool enabled) {
    HKEY handle = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &handle, nullptr) != ERROR_SUCCESS) return false;
    DWORD value = enabled ? 1 : 0;
    const auto result = RegSetValueExW(handle, L"PreferSteam", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(handle);
    return result == ERROR_SUCCESS;
}

INT_PTR CALLBACK options_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM) {
    if (message == WM_INITDIALOG) {
        CheckDlgButton(dialog, checkbox_id, read_steam() ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(wparam) == IDOK) {
            if (!write_steam(IsDlgButtonChecked(dialog, checkbox_id) == BST_CHECKED)) {
                MessageBoxW(dialog, L"Setarea nu a putut fi salvata.", L"HellBoost", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            DestroyWindow(dialog);
            return TRUE;
        }
        if (LOWORD(wparam) == IDCANCEL) { DestroyWindow(dialog); return TRUE; }
    }
    if (message == WM_CLOSE) { DestroyWindow(dialog); return TRUE; }
    if (message == WM_NCDESTROY && options_window == dialog) options_window = nullptr;
    return FALSE;
}

void __cdecl show_options() {
    if (options_window && IsWindow(options_window)) {
        ShowWindow(options_window, SW_SHOW);
        SetForegroundWindow(options_window);
        return;
    }
    HWND owner = GetActiveWindow();
    if (owner) {
        DWORD process = 0;
        GetWindowThreadProcessId(owner, &process);
        if (process != GetCurrentProcessId()) owner = nullptr;
    }
    // A modal DialogBox would stop engine frames and multiplayer networking.
    options_window = CreateDialogParamW(module, MAKEINTRESOURCEW(200), owner, options_proc, 0);
    if (options_window) {
        ShowWindow(options_window, SW_SHOW);
        UpdateWindow(options_window);
    }
}

template<class Function> Function method(unsigned index) {
    Function result = nullptr;
    static_assert(sizeof(result) == sizeof(void*), "Unexpected function pointer representation");
    std::memcpy(&result, original_vtable + index * sizeof(void*), sizeof(result));
    return result;
}

// ECX holds the GameUI 'this' pointer. The unused EDX parameter gives each
// fastcall thunk the stack layout of the corresponding x86 MSVC thiscall.
// Outgoing calls use the same unused EDX slot; no object implementation,
// compiler-generated vtable or pointer-to-member layout crosses the boundary.
void* __fastcall destroy(void*, void*, unsigned flags) {
    return method<void* (__fastcall*)(void*, void*, unsigned)>(0)(original_ui, nullptr, flags);
}
void __fastcall initialize(void*, void*, Factory* factories, int count) {
    method<void (__fastcall*)(void*, void*, Factory*, int)>(1)(original_ui, nullptr, factories, count);
}
void __fastcall start(void*, void*, void* engine, int version, void* system) {
    method<void (__fastcall*)(void*, void*, void*, int, void*)>(2)(original_ui, nullptr, engine, version, system);
    if (engine && version == 7) {
        // Verified GoldSrc cl_enginefuncs_s prefix: AddCommand is pointer 17.
        AddCommand add = nullptr;
        std::memcpy(&add, static_cast<const unsigned char*>(engine) + 17 * sizeof(void*), sizeof(add));
        if (add) {
            static char name[] = "hellboost_options";
            add(name, show_options);
        }
    }
}
void __fastcall shutdown(void*, void*) {
    if (options_window && IsWindow(options_window)) DestroyWindow(options_window);
    method<void (__fastcall*)(void*, void*)>(3)(original_ui, nullptr);
}
int __fastcall activate(void*, void*) { return method<int (__fastcall*)(void*, void*)>(4)(original_ui, nullptr); }
int __fastcall demo(void*, void*) { return method<int (__fastcall*)(void*, void*)>(5)(original_ui, nullptr); }
int __fastcall exclusive(void*, void*) { return method<int (__fastcall*)(void*, void*)>(6)(original_ui, nullptr); }
void __fastcall frame(void*, void*) { method<void (__fastcall*)(void*, void*)>(7)(original_ui, nullptr); }
void __fastcall connect(void*, void*, const char* game, int ip, int port) { method<void (__fastcall*)(void*, void*, const char*, int, int)>(8)(original_ui, nullptr, game, ip, port); }
void __fastcall disconnect(void*, void*) { method<void (__fastcall*)(void*, void*)>(9)(original_ui, nullptr); }
void __fastcall hide(void*, void*) {
    if (options_window && IsWindow(options_window)) ShowWindow(options_window, SW_HIDE);
    method<void (__fastcall*)(void*, void*)>(10)(original_ui, nullptr);
}
bool __fastcall active(void*, void*) { return method<bool (__fastcall*)(void*, void*)>(11)(original_ui, nullptr); }
void __fastcall loading_start(void*, void*, const char* type, const char* name) { method<void (__fastcall*)(void*, void*, const char*, const char*)>(12)(original_ui, nullptr, type, name); }
void __fastcall loading_finish(void*, void*, const char* type, const char* name) { method<void (__fastcall*)(void*, void*, const char*, const char*)>(13)(original_ui, nullptr, type, name); }
void __fastcall progress_start(void*, void*, const char* type, int steps) { method<void (__fastcall*)(void*, void*, const char*, int)>(14)(original_ui, nullptr, type, steps); }
int __fastcall progress_continue(void*, void*, int point, float fraction) { return method<int (__fastcall*)(void*, void*, int, float)>(15)(original_ui, nullptr, point, fraction); }
void __fastcall progress_stop(void*, void*, bool error, const char* reason, const char* extended) { method<void (__fastcall*)(void*, void*, bool, const char*, const char*)>(16)(original_ui, nullptr, error, reason, extended); }
int __fastcall progress_text(void*, void*, const char* text) { return method<int (__fastcall*)(void*, void*, const char*)>(17)(original_ui, nullptr, text); }
void __fastcall secondary(void*, void*, float value) { method<void (__fastcall*)(void*, void*, float)>(18)(original_ui, nullptr, value); }
void __fastcall secondary_text(void*, void*, const char* text) { method<void (__fastcall*)(void*, void*, const char*)>(19)(original_ui, nullptr, text); }
void __fastcall validate(void*, void*, bool force, bool connect_state) { method<void (__fastcall*)(void*, void*, bool, bool)>(20)(original_ui, nullptr, force, connect_state); }
void __fastcall login_failure(void*, void*, int code, const char* user) { method<void (__fastcall*)(void*, void*, int, const char*)>(21)(original_ui, nullptr, code, user); }

template<class Function> void* slot(Function function) {
    void* result = nullptr;
    static_assert(sizeof(result) == sizeof(function), "Unexpected thunk pointer representation");
    std::memcpy(&result, &function, sizeof(result));
    return result;
}

void* table[22]{};
struct Proxy { void** vtable = table; } proxy;

BOOL CALLBACK load_original(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[32768]{};
    const DWORD length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768) return FALSE;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return FALSE;
    const auto prefix = static_cast<std::size_t>(slash + 1 - path);
    constexpr wchar_t suffix[] = L"HellBoost.GameUI.original.dll";
    if (prefix + sizeof(suffix) / sizeof(wchar_t) > 32768) return FALSE;
    std::memcpy(path + prefix, suffix, sizeof(suffix));
    original_module = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!original_module) return FALSE;
    const auto address = GetProcAddress(original_module, "CreateInterface");
    std::memcpy(&factory, &address, sizeof(factory));
    if (factory) original_ui = factory("GameUI007", nullptr);
    if (!original_ui) {
        factory = nullptr;
        FreeLibrary(original_module);
        original_module = nullptr;
        return FALSE;
    }
    std::memcpy(&original_vtable, original_ui, sizeof(original_vtable));
    void* entries[] = {
        slot(destroy), slot(initialize), slot(start), slot(shutdown), slot(activate), slot(demo), slot(exclusive), slot(frame),
        slot(connect), slot(disconnect), slot(hide), slot(active), slot(loading_start), slot(loading_finish), slot(progress_start),
        slot(progress_continue), slot(progress_stop), slot(progress_text), slot(secondary), slot(secondary_text), slot(validate), slot(login_failure)
    };
    std::memcpy(table, entries, sizeof(table));
    return TRUE;
}
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, int* code) {
    if (!name || !InitOnceExecuteOnce(&once, load_original, nullptr, nullptr)) {
        if (code) *code = 1;
        return nullptr;
    }
    if (std::strcmp(name, "GameUI007") == 0) {
        if (code) *code = 0;
        return &proxy;
    }
    return factory(name, code);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) module = instance;
    return TRUE;
}
