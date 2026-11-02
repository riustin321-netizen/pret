from pathlib import Path
import tempfile
import unittest

from launcher.runtime import find_local_game, launch_command, validate_address


class RuntimeTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.steam = self.root / "Steam Files" / "steam.exe"
        self.steam.parent.mkdir()
        self.steam.touch()
        self.local = self.root / "client" / "hl.exe"
        self.local.parent.mkdir()
        self.local.touch()
        (self.local.parent / "cstrike").mkdir()

    def test_steam_has_priority_and_keeps_arguments_separate(self):
        command, cwd, mode = launch_command(self.steam, self.local, "Player One", True, "play.example.com:27015")
        self.assertEqual(command[:3], [str(self.steam), "-applaunch", "10"])
        self.assertEqual(command[-2:], ["+connect", "play.example.com:27015"])
        self.assertIn("Player One", command)
        self.assertEqual(cwd, str(self.steam.parent))
        self.assertEqual(mode, "Steam")

    def test_no_steam_launches_local_in_game_directory(self):
        command, cwd, mode = launch_command(None, self.local, "Player", False)
        self.assertEqual(command[0], str(self.local))
        self.assertNotIn("-applaunch", command)
        self.assertEqual(cwd, str(self.local.parent))
        self.assertEqual(mode, "local / non-Steam")

    def test_discovers_game_beside_launcher_or_client_subdirectory(self):
        self.assertEqual(find_local_game(self.root), self.local.resolve())
        adjacent = self.root / "hl.exe"
        adjacent.touch()
        (self.root / "cstrike").mkdir()
        self.assertEqual(find_local_game(self.root), adjacent.resolve())

    def test_missing_cstrike_is_not_a_game(self):
        (self.local.parent / "cstrike").rmdir()
        self.assertIsNone(find_local_game(self.root))
        with self.assertRaises(ValueError):
            launch_command(None, self.local, "Player", True)

    def test_saved_location(self):
        self.assertEqual(find_local_game(self.root / "elsewhere", str(self.local)), self.local.resolve())

    def test_untrusted_addresses(self):
        for value in (None, "host:0", "host:65536", "host:27015;quit", "host:27015\n", "https://host:27015"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_address(value)
