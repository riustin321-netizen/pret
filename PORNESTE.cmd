@echo off
cd /d "%~dp0"
py -3 launcher\main.py
if errorlevel 1 (
    echo Este necesar Python 3.11 sau mai nou cu Tcl/Tk si Python Launcher.
    pause
)
