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
echo Native Win32 library and tests built successfully.
exit /b 0
