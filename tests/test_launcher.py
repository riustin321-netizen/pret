import unittest
from launcher.main import launch_arguments


class ArgumentsTest(unittest.TestCase):
    def test_steam_launch(self):
        args = launch_arguments("Player One", True, "play.example.com:27015")
        self.assertEqual(args[:2], ["-applaunch", "10"])
        self.assertIn("-windowed", args)
        self.assertEqual(args[-2:], ["+connect", "play.example.com:27015"])

    def test_reject_console_injection(self):
        for name, server in [("Player;quit", ""), ("Player", "host:27015;quit"), ("Player", "host:65536")]:
            with self.assertRaises(ValueError):
                launch_arguments(name, False, server)

    def test_fullscreen_without_server(self):
        args = launch_arguments("Player", False)
        self.assertIn("-fullscreen", args)
        self.assertNotIn("+connect", args)


if __name__ == "__main__":
    unittest.main()
