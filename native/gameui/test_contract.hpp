#pragma once
#include <cstdint>

using FixtureFactory = void* (__cdecl*)(const char*, int*);
// Interoperability contract used only by the MSVC ABI fixture, not an SDK implementation.
class GameUI007Contract {
public:
    virtual ~GameUI007Contract() = default;
    virtual void Initialize(FixtureFactory*, int) = 0;
    virtual void Start(void*, int, void*) = 0;
    virtual void Shutdown() = 0;
    virtual int ActivateGameUI() = 0;
    virtual int ActivateDemoUI() = 0;
    virtual int HasExclusiveInput() = 0;
    virtual void RunFrame() = 0;
    virtual void ConnectToServer(const char*, int, int) = 0;
    virtual void DisconnectFromServer() = 0;
    virtual void HideGameUI() = 0;
    virtual bool IsGameUIActive() = 0;
    virtual void LoadingStarted(const char*, const char*) = 0;
    virtual void LoadingFinished(const char*, const char*) = 0;
    virtual void StartProgressBar(const char*, int) = 0;
    virtual int ContinueProgressBar(int, float) = 0;
    virtual void StopProgressBar(bool, const char*, const char*) = 0;
    virtual int SetProgressBarStatusText(const char*) = 0;
    virtual void SetSecondaryProgressBar(float) = 0;
    virtual void SetSecondaryProgressBarText(const char*) = 0;
    virtual void ValidateCDKey(bool, bool) = 0;
    virtual void OnDisconnectFromServer(int, const char*) = 0;
};

struct FixtureState {
    unsigned calls[22]{};
    unsigned bad_arguments = 0;
};
