@echo off
setlocal
cd /d "%~dp0"
if exist "%~dp0HellBoost.exe" (
    start "" "%~dp0HellBoost.exe"
    exit /b 0
)
py -3 launcher\main.py
if errorlevel 1 (
    echo Este necesar Python 3.11 sau mai nou cu Tcl/Tk si Python Launcher.
    pause
)
