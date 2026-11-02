"""HellBoost external platform browser; no in-game hooks or join reports yet."""
import json
import os
from pathlib import Path
import queue
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

try:
    from .platform_api import fetch_servers
    from .runtime import (find_local_game, find_steam, game_arguments,
                          launch_command, launcher_directory, valid_local_game, validate_address)
except ImportError:
    from platform_api import fetch_servers
    from runtime import (find_local_game, find_steam, game_arguments,
                         launch_command, launcher_directory, valid_local_game, validate_address)


def launch_arguments(name, windowed, server=""):
    """Compatibility wrapper for the first prototype's Steam arguments."""
    return ["-applaunch", "10", *game_arguments(name, windowed, server)]


class Launcher:
    def __init__(self, root):
        self.root = root
        self.path = Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "HellBoost" / "settings.json"
        settings = {}
        try:
            settings = json.loads(self.path.read_text(encoding="utf-8"))
            if not isinstance(settings, dict):
                settings = {}
        except (OSError, ValueError):
            pass
        self.name = tk.StringVar(value=str(settings.get("name", "Player")))
        self.server = tk.StringVar(value=str(settings.get("server", "")))
        self.windowed = tk.BooleanVar(value=settings.get("windowed", True) is True)
        self.local = str(settings.get("hl_path", ""))
        self.favorites = self.read_addresses(settings.get("favorites", []))
        self.history = self.read_addresses(settings.get("history", []))
        self.online = []
        self.results = queue.Queue()
        self.loading = False
        self.status = tk.StringVar(value="Pregătit. Steam are prioritate dacă este instalat.")
        root.title("HellBoost — Browser și launcher 0.2")
        root.geometry("960x680")
        root.minsize(760, 600)
        frame = ttk.Frame(root, padding=20)
        frame.pack(fill="both", expand=True)
        ttk.Label(frame, text="HELLBOOST", font=("Segoe UI", 25, "bold")).pack(anchor="w")
        ttk.Label(frame, text="Browser extern · țintă Steam legacy 8684 · pornire Steam / local").pack(anchor="w", pady=(0, 12))
        self.tabs = ttk.Notebook(frame)
        self.tabs.pack(fill="both", expand=True)
        self.tables = {}
        for key, title in (("internet", "Internet"), ("favorites", "Favorite"), ("history", "Istoric")):
            page = ttk.Frame(self.tabs)
            self.tabs.add(page, text=title)
            table = ttk.Treeview(page, columns=("rank", "name", "players", "promo", "address"), show="headings", selectmode="browse")
            for column, label, width in (("rank", "#", 45), ("name", "Server", 300), ("players", "Jucători", 80), ("promo", "Boost", 80), ("address", "Adresă", 200)):
                table.heading(column, text=label)
                table.column(column, width=width, minwidth=40)
            scroll = ttk.Scrollbar(page, orient="vertical", command=table.yview)
            table.configure(yscrollcommand=scroll.set)
            scroll.pack(side="right", fill="y")
            table.pack(fill="both", expand=True)
            table.bind("<<TreeviewSelect>>", self.select_server)
            table.bind("<Double-1>", lambda event: self.play())
            self.tables[key] = table
        actions = ttk.Frame(frame)
        actions.pack(fill="x", pady=10)
        self.refresh_button = ttk.Button(actions, text="REÎNCARCĂ LISTA", command=self.refresh)
        self.refresh_button.pack(side="left")
        ttk.Button(actions, text="ADAUGĂ / ȘTERGE FAVORIT", command=self.toggle_favorite).pack(side="left", padx=8)
        ttk.Button(actions, text="ALEGE HL.EXE LOCAL", command=self.choose_local).pack(side="left")
        row = ttk.Frame(frame)
        row.pack(fill="x")
        ttk.Label(row, text="Nume:").pack(side="left")
        ttk.Entry(row, textvariable=self.name, width=22).pack(side="left", padx=8)
        ttk.Label(row, text="Server:").pack(side="left")
        ttk.Entry(row, textvariable=self.server).pack(side="left", padx=8, fill="x", expand=True)
        ttk.Checkbutton(row, text="În fereastră", variable=self.windowed).pack(side="left")
        ttk.Button(frame, text="PORNEȘTE CS 1.6", command=self.play).pack(fill="x", pady=12)
        ttk.Label(frame, textvariable=self.status, wraplength=850).pack(anchor="w")
        ttk.Label(frame, text="Prototip: lista este în launcher. Find Servers din joc și raportarea intrărilor nu sunt integrate.", wraplength=850).pack(anchor="w", pady=(8, 0))
        self.render()
        self.poll_id = root.after(100, self.poll)
        root.protocol("WM_DELETE_WINDOW", self.close)

    def close(self):
        self.root.after_cancel(self.poll_id)
        self.root.destroy()

    @staticmethod
    def read_addresses(values):
        result = []
        if isinstance(values, list):
            for value in values[:1000]:
                try:
                    validate_address(value)
                    if value not in result:
                        result.append(value)
                except ValueError:
                    pass
        return result

    def render(self):
        by_address = {item["address"]: item for item in self.online}
        for key, table in self.tables.items():
            table.delete(*table.get_children())
            rows = self.online if key == "internet" else [by_address.get(address, {"address": address, "hostname": address}) for address in getattr(self, key)]
            for index, item in enumerate(rows):
                players = f'{item["players"]}/{item["max_players"]}' if "players" in item else "—"
                table.insert("", "end", iid=str(index), values=(item.get("rank", "—"), item["hostname"], players, item.get("promo", ""), item["address"]))

    def select_server(self, event):
        selected = event.widget.selection()
        if selected:
            self.server.set(event.widget.item(selected[0], "values")[-1])

    def refresh(self):
        if self.loading:
            return
        self.loading = True
        self.refresh_button.configure(state="disabled")
        self.status.set("Se încarcă lista HellBoost…")

        def worker():
            try:
                self.results.put((True, fetch_servers()))
            except Exception as exc:
                self.results.put((False, str(exc)))

        threading.Thread(target=worker, daemon=True).start()

    def poll(self):
        try:
            success, result = self.results.get_nowait()
            self.loading = False
            self.refresh_button.configure(state="normal")
            if success:
                self.online = result
                self.render()
                self.status.set(f"{len(result)} servere încărcate în ordinea platformei.")
            else:
                self.status.set(result)
        except queue.Empty:
            pass
        self.poll_id = self.root.after(100, self.poll)

    def persist(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix(".tmp")
        temporary.write_text(json.dumps({"name": self.name.get(), "server": self.server.get(), "windowed": self.windowed.get(), "hl_path": self.local, "favorites": self.favorites, "history": self.history}, ensure_ascii=False, indent=2), encoding="utf-8")
        temporary.replace(self.path)

    def toggle_favorite(self):
        try:
            address = validate_address(self.server.get().strip())
            old = list(self.favorites)
            if address in self.favorites:
                self.favorites.remove(address)
            else:
                self.favorites.append(address)
            try:
                self.persist()
            except OSError:
                self.favorites = old
                raise
            self.render()
        except (ValueError, OSError) as exc:
            messagebox.showerror("HellBoost", str(exc))

    def choose_local(self):
        chosen = filedialog.askopenfilename(title="Selectează hl.exe", filetypes=[("Half-Life", "hl.exe")])
        if not chosen:
            return False
        path = Path(chosen).resolve()
        if not valid_local_game(path):
            messagebox.showerror("HellBoost", "Selectează hl.exe dintr-un folder care conține cstrike.")
            return False
        try:
            self.local = str(path)
            self.persist()
            self.status.set("Instalare locală salvată. Steam are prioritate dacă este instalat.")
            return True
        except OSError as exc:
            messagebox.showerror("HellBoost", str(exc))
            return False

    def play(self):
        try:
            if os.name != "nt":
                raise ValueError("Pornirea jocului este disponibilă pe Windows.")
            address = self.server.get().strip()
            game_arguments(self.name.get().strip(), self.windowed.get(), address)
            steam = find_steam()
            local = find_local_game(launcher_directory(), self.local)
            if not steam and not local:
                if not self.choose_local():
                    return
                local = Path(self.local)
            command, directory, mode = launch_command(steam, local, self.name.get().strip(), self.windowed.get(), address)
            self.persist()
            subprocess.Popen(command, cwd=directory, shell=False)
            if address:
                self.history = [address, *[item for item in self.history if item != address]][:100]
                try:
                    self.persist()
                except OSError:
                    self.status.set("Pornire solicitată; istoricul nu a putut fi salvat.")
                    self.render()
                    return
                self.render()
            self.status.set(f"Pornire solicitată prin {mode}. Intrarea efectivă nu este încă monitorizată.")
        except (OSError, ValueError) as exc:
            messagebox.showerror("HellBoost", str(exc))


if __name__ == "__main__":
    root = tk.Tk()
    Launcher(root)
    root.mainloop()
