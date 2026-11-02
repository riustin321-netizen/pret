"""Exercise the real PHP HTTP route against a fixture DB adapter, not production."""
import http.client
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time
import unittest

PHP = os.environ.get("HELLBOOST_PHP_EXECUTABLE") or shutil.which("php")
ROOT = Path(__file__).resolve().parent.parent


@unittest.skipUnless(PHP, "PHP CLI is required for HTTP endpoint integration tests")
class PHPEndpointTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.directory = Path(cls.temp.name)
        (cls.directory / "inc").mkdir()
        source = ROOT / "integrations/hellboost"
        shutil.copy(source / "master_nextclient.php", cls.directory)
        shutil.copy(source / "inc/master_nextclient_format.php", cls.directory / "inc")
        (cls.directory / "inc/db.php").write_text("""<?php
function ranked_servers($where) {
    if ($where !== 's.online = 1') throw new RuntimeException('wrong filter');
    if (isset($_GET['failure'])) throw new RuntimeException('private fixture error');
    return [
        ['ip'=>'192.0.2.1', 'port'=>27015, 'online'=>1, 'country'=>'ro'],
        ['ip'=>'198.51.100.2', 'port'=>27016, 'online'=>1],
    ];
}
""", encoding="utf-8")
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            cls.port = probe.getsockname()[1]
        cls.process = subprocess.Popen(
            [PHP, "-n", "-S", f"127.0.0.1:{cls.port}", "-t", str(cls.directory)],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if cls.process.poll() is not None:
                cls.tearDownClass()
                raise RuntimeError("PHP HTTP fixture server failed to start")
            try:
                with socket.create_connection(("127.0.0.1", cls.port), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.05)
        cls.tearDownClass()
        raise RuntimeError("PHP HTTP fixture server readiness timed out")

    @classmethod
    def tearDownClass(cls):
        if cls.process.poll() is None:
            cls.process.terminate()
            cls.process.wait(timeout=5)
        cls.temp.cleanup()

    def request(self, method="GET", query="", body=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=5)
        self.addCleanup(connection.close)
        connection.request(method, "/master_nextclient.php" + query, body, {"Content-Type": "application/json"})
        response = connection.getresponse()
        return response.status, response.getheader("Content-Type"), response.read()

    def test_nextclient_get_with_json_body_returns_root_array(self):
        status, content_type, body = self.request(body='{"method":"server_list","data":{"extended":true}}')
        self.assertEqual(status, 200)
        self.assertIn("application/json", content_type)
        self.assertEqual(json.loads(body), [
            {"address": "192.0.2.1:27015", "country": "RO"},
            {"address": "198.51.100.2:27016"},
        ])

    def test_rejects_non_get_methods(self):
        status, _, body = self.request("POST")
        self.assertEqual(status, 405)
        self.assertEqual(json.loads(body)["error"], "method_not_allowed")

    def test_database_failure_is_503_not_empty_success(self):
        status, _, body = self.request(query="?failure=1")
        self.assertEqual(status, 503)
        self.assertEqual(json.loads(body), {"error": "temporarily_unavailable"})
        self.assertNotIn(b"private fixture", body)
