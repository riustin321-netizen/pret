"""Game discovery and commands; importing this module never launches a game."""
import os
from pathlib import Path
import re
import sys


def launcher_directory():
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent.parent


def validate_address(address):
    if not isinstance(address, str) or not re.fullmatch(
        r"[A-Za-z0-9](?:[A-Za-z0-9.-]{0,251}[A-Za-z0-9])?:[0-9]{1,5}", address
    ):
        raise ValueError("Serverul trebuie să fie de forma domeniu:port sau IP:port.")
    if not 1 <= int(address.rsplit(":", 1)[1]) <= 65535:
        raise ValueError("Port invalid.")
    return address


def game_arguments(name, windowed, server=""):
    if not isinstance(name, str) or not re.fullmatch(r"[\w .-]{1,32}", name):
        raise ValueError("Numele trebuie să aibă 1–32 caractere: litere, cifre, spații, . - _")
    args = ["-game", "cstrike", "-windowed" if windowed else "-fullscreen", "+name", name]
    if server:
        args += ["+connect", validate_address(server)]
    return args


def find_steam():
    if os.name != "nt":
        return None
    import winreg
    candidates = []
    for hive, key, field, directory in (
        (winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamExe", False),
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Valve\Steam", "InstallPath", True),
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath", True),
    ):
        try:
            with winreg.OpenKey(hive, key) as handle:
                value = winreg.QueryValueEx(handle, field)[0]
            if isinstance(value, str):
                path = Path(value)
                candidates.append(path / "steam.exe" if directory else path)
        except OSError:
            continue
    for variable in ("ProgramFiles(x86)", "ProgramFiles"):
        if os.environ.get(variable):
            candidates.append(Path(os.environ[variable]) / "Steam" / "steam.exe")
    return next((p.resolve() for p in candidates if p.is_file() and p.name.lower() == "steam.exe"), None)


def valid_local_game(path):
    return path.name.lower() == "hl.exe" and path.is_file() and (path.parent / "cstrike").is_dir()


def find_local_game(directory, saved_path=""):
    candidates = [Path(directory) / "hl.exe", Path(directory) / "client" / "hl.exe"]
    if saved_path:
        candidates.append(Path(saved_path))
    return next((p.resolve() for p in candidates if valid_local_game(p)), None)


def launch_command(steam, local, name, windowed, server=""):
    args = game_arguments(name, windowed, server)
    if steam:
        path = Path(steam)
        if not path.is_file() or path.name.lower() != "steam.exe":
            raise ValueError("Instalarea Steam nu este validă.")
        return [str(path), "-applaunch", "10", *args], str(path.parent), "Steam"
    if local and valid_local_game(Path(local)):
        path = Path(local)
        return [str(path), *args], str(path.parent), "local / non-Steam"
    raise ValueError("Steam nu a fost găsit. Selectează hl.exe din instalarea ta locală.")
