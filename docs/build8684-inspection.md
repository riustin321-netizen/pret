# Static inspection of the uploaded cs.zip

The archive was transferred successfully (22,940,449 bytes, 33 entries). No uploaded executable or DLL was executed, loaded, modified, or added to the repository/download packages.

`hw.dll` is present, a 32-bit x86 PE DLL with SHA-256 `be45f76049a133392423679d334c69c8e1e7e82dc873eebdd229ea0341ba1b10`. Its PE timestamp is 2020-08-19; embedded version strings include `Exe build: 19:53:27 Aug  3 2020 (%i)`. This is static evidence, not verification that it is an unmodified official Steam binary or that the reported runtime build is 8684.

The engine imports `steam_api.dll`, exports the launcher interface identifier `VENGINE_LAUNCHER_API_VERSION002`, and contains the GameUI lookup string `cl_dlls\\GameUI.dll`. These strings do not establish the complete GameUI ABI or a supported plugin loader.

The archive does not contain `GameUI.dll`, `cstrike/cl_dlls/client.dll`, or game UI resources. It includes files named `SmartSteamEmu.asi`, `SmartSteamEmu.dll`, and `SmartSteamEmu.ini`; their presence means the archive cannot be assumed to be a clean Steam installation. The emulator was not executed or configured. HellBoost's Steam launch mode continues to use the official installed Steam client.

The next required input for analysis of the actual in-game options menu is `GameUI.dll` from the target build's game directory, commonly under `valve/cl_dlls` or `cstrike/cl_dlls`. Export inspection must precede any choice of ABI forwarding or menu extension. No game integration or Steam compatibility beyond the existing launcher has been implemented by this inspection.

## Subsequent GameUI upload

The separately uploaded `GameUI.dll` is 842,648 bytes, PE32/i386, SHA-256 `684fb5615f0c3ab7d07fcf5503eb214d064c70e25eb0eb22bbf043b4d73c162f`. Its PE timestamp is 2020-08-04. Its only named export is `CreateInterface` (RVA `0x23c00`), and the binary contains `GameUI007`, `VEngineVGui001`, `Resource/GameMenu.res`, and the `engine ` menu command prefix. It contains the stock Multiplayer/Keyboard/Mouse/Audio/Video/Voice options resource names, but no confirmed custom Misc page.

The public declaration of `GameUI007` inspected as an ABI reference has a virtual base destructor and 21 methods, including initialization, frame callbacks, server connection notifications and loading progress callbacks. A forwarding implementation must verify the actual target vtable layout and x86 calling conventions; using a MinGW C++ class directly as a drop-in MSVC C++ interface is not established as compatible. No original binary was executed or redistributed, and no engine/game UI DLL was patched.

The HellBoost browser exposes a persistent Steam preference for the next launch. Subsequently, an independent experimental GameUI bridge was implemented in `native/gameui`: it forwards the interface to the original library and registers an in-process options panel. The original GameUI binary remains unmodified. Windows tests with a simulated original validated all 21 methods, the panel and checkbox, but the real game's rendering/input behavior is still untested. Loading callbacks have not been declared proof of full player entry.
