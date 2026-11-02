import os
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch

from launcher.main import Launcher


@unittest.skipUnless(os.name == "nt" or os.environ.get("DISPLAY"), "A graphical display is required for Tk UI tests")
class LauncherUITest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        environment = patch.dict(os.environ, {"LOCALAPPDATA": self.temp.name})
        environment.start()
        self.addCleanup(environment.stop)
        self.root = tk.Tk()
        self.root.withdraw()
        self.app = Launcher(self.root)
        self.addCleanup(self.app.close)

    def wait_for_refresh(self):
        deadline = time.monotonic() + 3
        while self.app.loading and time.monotonic() < deadline:
            self.root.update()
            time.sleep(0.01)
        self.assertFalse(self.app.loading, "UI did not receive background fetch result")

    def test_render_select_and_persist_favorite(self):
        self.app.online = [
            {"rank": 1, "address": "top.example.com:27015", "hostname": "TOP", "players": 1, "max_players": 32, "promo": "top"},
            {"rank": 2, "address": "busy.example.com:27015", "hostname": "Busy", "players": 30, "max_players": 32, "promo": ""},
        ]
        self.app.render()
        table = self.app.tables["internet"]
        self.assertEqual(table.item("0", "values")[1], "TOP")
        table.selection_set("1")
        table.event_generate("<<TreeviewSelect>>")
        self.root.update()
        self.assertEqual(self.app.server.get(), "busy.example.com:27015")
        self.app.toggle_favorite()
        self.assertTrue(self.app.path.is_file())
        self.assertEqual(self.app.favorites, ["busy.example.com:27015"])
        self.assertEqual(len(self.app.tables["favorites"].get_children()), 1)
        self.app.toggle_favorite()
        self.assertEqual(self.app.favorites, [])

    def test_async_refresh_and_visible_failure_without_fake_rows(self):
        with patch("launcher.main.fetch_servers", return_value=[]):
            self.app.refresh()
            self.wait_for_refresh()
        self.assertIn("0 servere", self.app.status.get())
        with patch("launcher.main.fetch_servers", side_effect=ValueError("API indisponibil")):
            self.app.refresh()
            self.wait_for_refresh()
        self.assertEqual(self.app.status.get(), "API indisponibil")
        self.assertEqual(self.app.tables["internet"].get_children(), ())
