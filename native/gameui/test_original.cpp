#include "test_contract.hpp"
#include <cstring>

namespace {
FixtureState state;
bool equal(const char* a, const char* b) { return a && std::strcmp(a, b) == 0; }
void record(unsigned index, bool valid = true) { ++state.calls[index]; if (!valid) ++state.bad_arguments; }
class Fixture final : public GameUI007Contract {
public:
    void Initialize(FixtureFactory*, int count) override { record(1, count == 3); }
    void Start(void* engine, int version, void* system) override { record(2, engine && version == 7 && reinterpret_cast<std::uintptr_t>(system) == 0x1234); }
    void Shutdown() override { record(3); }
    int ActivateGameUI() override { record(4); return 24; }
    int ActivateDemoUI() override { record(5); return 25; }
    int HasExclusiveInput() override { record(6); return 26; }
    void RunFrame() override { record(7); }
    void ConnectToServer(const char* game, int ip, int port) override { record(8, equal(game, "cstrike") && ip == 0x01020304 && port == 27015); }
    void DisconnectFromServer() override { record(9); }
    void HideGameUI() override { record(10); }
    bool IsGameUIActive() override { record(11); return true; }
    void LoadingStarted(const char* type, const char* name) override { record(12, equal(type, "map") && equal(name, "de_dust2")); }
    void LoadingFinished(const char* type, const char* name) override { record(13, equal(type, "map") && equal(name, "de_dust2")); }
    void StartProgressBar(const char* type, int steps) override { record(14, equal(type, "connect") && steps == 42); }
    int ContinueProgressBar(int point, float fraction) override { record(15, point == 3 && fraction == 0.25f); return 27; }
    void StopProgressBar(bool error, const char* reason, const char* extended) override { record(16, error && equal(reason, "no") && equal(extended, "x")); }
    int SetProgressBarStatusText(const char* text) override { record(17, equal(text, "test")); return 28; }
    void SetSecondaryProgressBar(float value) override { record(18, value == 0.5f); }
    void SetSecondaryProgressBarText(const char* text) override { record(19, equal(text, "secondary")); }
    void ValidateCDKey(bool force, bool connected) override { record(20, force && !connected); }
    void OnDisconnectFromServer(int code, const char* user) override { record(21, code == 99 && equal(user, "user")); }
} fixture;
}

extern "C" __declspec(dllexport) void* __cdecl CreateInterface(const char* name, int* code) {
    if (name && std::strcmp(name, "GameUI007") == 0) { if (code) *code = 0; return &fixture; }
    if (name && std::strcmp(name, "HellBoostFixture001") == 0) { if (code) *code = 0; return &state; }
    if (code) *code = 1;
    return nullptr;
}
