@echo off
setlocal
cd /d "%~dp0"
set "HB_VS_INSTALL="
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    echo Install Visual Studio 2022 Build Tools with Desktop development with C++.
    exit /b 1
)
for /f "usebackq tokens=*" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "HB_VS_INSTALL=%%I"
if not defined HB_VS_INSTALL exit /b 1
call "%HB_VS_INSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
if errorlevel 1 exit /b 1
if not exist build mkdir build
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /MT /DHB_BUILD /LD native\hellboost_client.cpp /Fobuild\hellboost_client.obj /Febuild\HellBoostClient.dll /link /DEF:native\hellboost_client.def /IMPLIB:build\HellBoostClient.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /MT native\tests\abi_test.cpp build\HellBoostClient.lib /Fobuild\abi_test.obj /Febuild\abi_test.exe
if errorlevel 1 exit /b 1
build\abi_test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /MT /UNDEBUG native\tests\connection_tracker_test.cpp /Fobuild\tracker_test.obj /Febuild\tracker_test.exe
if errorlevel 1 exit /b 1
build\tracker_test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /MT native\browser\model.cpp native\tests\browser_test.cpp /Fobuild\ /Febuild\browser_test.exe
if errorlevel 1 exit /b 1
build\browser_test.exe
if errorlevel 1 exit /b 1
rc /nologo /fobuild\browser_resources.res native\browser\resources.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /MT /DUNICODE /D_UNICODE /DNOMINMAX /D_WIN32_WINNT=0x0601 native\browser\main.cpp native\browser\windows.cpp native\browser\model.cpp build\browser_resources.res /Fobuild\ /Febuild\HellBoost.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib advapi32.lib comctl32.lib comdlg32.lib winhttp.lib
if errorlevel 1 exit /b 1
echo HellBoost native Win32 browser, library and tests built successfully.
exit /b 0
