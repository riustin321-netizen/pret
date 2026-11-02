#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "test_contract.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #condition); std::exit(1); } } while (false)
namespace {
using Command = void (__cdecl*)();
Command command = nullptr;
int __cdecl add_command(char* name, Command handler) {
    CHECK(name && std::strcmp(name, "hellboost_options") == 0);
    command = handler;
    return 1;
}
struct EnginePrefix { void* reserved[17]{}; int (__cdecl* add)(char*, Command) = add_command; };
}

int wmain(int argc, wchar_t** argv) {
    CHECK(argc == 2);
    HMODULE module = LoadLibraryW(argv[1]);
    CHECK(module);
    const auto address = GetProcAddress(module, "CreateInterface");
    FixtureFactory factory = nullptr;
    std::memcpy(&factory, &address, sizeof(factory));
    CHECK(factory);
    int code = -1;
    CHECK(!factory(nullptr, &code) && code == 1);
    auto ui = static_cast<GameUI007Contract*>(factory("GameUI007", &code));
    CHECK(ui && code == 0);
    CHECK(factory("GameUI007", nullptr) == ui);
    CHECK(!factory("MissingInterface001", &code) && code == 1);
    auto state = static_cast<FixtureState*>(factory("HellBoostFixture001", nullptr));
    CHECK(state);
    EnginePrefix engine;
    ui->Initialize(nullptr, 3);
    ui->Start(&engine, 7, reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x1234)));
    CHECK(command);
    CHECK(ui->ActivateGameUI() == 24);
    CHECK(ui->ActivateDemoUI() == 25);
    CHECK(ui->HasExclusiveInput() == 26);
    ui->RunFrame();
    ui->ConnectToServer("cstrike", 0x01020304, 27015);
    ui->DisconnectFromServer();
    ui->HideGameUI();
    CHECK(ui->IsGameUIActive());
    ui->LoadingStarted("map", "de_dust2");
    ui->LoadingFinished("map", "de_dust2");
    ui->StartProgressBar("connect", 42);
    CHECK(ui->ContinueProgressBar(3, 0.25f) == 27);
    ui->StopProgressBar(true, "no", "x");
    CHECK(ui->SetProgressBarStatusText("test") == 28);
    ui->SetSecondaryProgressBar(0.5f);
    ui->SetSecondaryProgressBarText("secondary");
    ui->ValidateCDKey(true, false);
    ui->OnDisconnectFromServer(99, "user");

    std::thread dialog_thread([] {
        command();
        HWND dialog = FindWindowW(nullptr, L"HellBoost / Options");
        CHECK(dialog);
        MSG message{};
        while (IsWindow(dialog)) {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (!IsDialogMessageW(dialog, &message)) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    HWND dialog = nullptr;
    HWND checkbox = nullptr;
    for (unsigned i = 0; i < 250; ++i) {
        dialog = FindWindowW(nullptr, L"HellBoost / Options");
        if (dialog) checkbox = GetDlgItem(dialog, 201);
        if (dialog && checkbox && IsWindowVisible(dialog)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK(dialog);
    DWORD owner = 0;
    GetWindowThreadProcessId(dialog, &owner);
    CHECK(owner == GetCurrentProcessId());
    CHECK(checkbox);
    const auto before = SendMessageW(checkbox, BM_GETCHECK, 0, 0);
    SendMessageW(checkbox, BM_CLICK, 0, 0);
    CHECK(SendMessageW(checkbox, BM_GETCHECK, 0, 0) != before);
    SendMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
    dialog_thread.join();
    ui->Shutdown();
    CHECK(state->bad_arguments == 0);
    for (unsigned i = 1; i <= 21; ++i) CHECK(state->calls[i] == 1);
    FreeLibrary(module);
    std::puts("GameUI007 bridge: all 21 methods forwarded; in-process options checkbox opened and toggled. No game or Steam authentication tested.");
}
