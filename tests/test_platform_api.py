import io
import json
import unittest
from unittest.mock import patch
from urllib.error import URLError
from urllib.request import Request

from launcher.platform_api import APIError, MAX_BODY, SameOriginRedirect, fetch_servers, parse_servers


def fixture():
    return {"servers": [
        {"rank": 1, "address": "top.example.com:27015", "hostname": "TOP", "players": 3, "max_players": 32, "promo": "top"},
        {"rank": 2, "address": "busy.example.com:27015", "hostname": "Busy", "players": 30, "max_players": 32, "promo": ""},
    ]}


class PlatformAPITest(unittest.TestCase):
    def test_preserves_platform_order_even_when_players_differ(self):
        rows = parse_servers(json.dumps(fixture()))
        self.assertEqual([r["hostname"] for r in rows], ["TOP", "Busy"])

    def test_removes_duplicates_and_controls(self):
        data = fixture()
        data["servers"][0]["hostname"] = "TOP\n\x00"
        data["servers"].append(dict(data["servers"][0]))
        rows = parse_servers(json.dumps(data))
        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0]["hostname"], "TOP")

    def test_rejects_invalid_body_or_schema(self):
        bodies = [b"<html>error</html>", b'{}', b'[]', b'{"servers":[null]}', b'x' * (MAX_BODY + 1)]
        for body in bodies:
            with self.subTest(body=body[:40]), self.assertRaises(APIError):
                parse_servers(body)

    def test_rejects_malicious_address_and_wrong_types(self):
        for key, value in (("address", "host:27015;quit"), ("players", True), ("rank", -1), ("hostname", [])):
            data = fixture()
            data["servers"][0][key] = value
            with self.subTest(key=key), self.assertRaises(APIError):
                parse_servers(json.dumps(data))

    def test_https_fetch_uses_existing_endpoint_and_bounded_read(self):
        response = io.BytesIO(json.dumps(fixture()).encode())
        with patch("launcher.platform_api.build_opener") as opener:
            opener.return_value.open.return_value = response
            self.assertEqual(len(fetch_servers()), 2)
            args, kwargs = opener.return_value.open.call_args
            self.assertEqual(args[0].full_url, "https://hellboost.net/launcher?a=list")
            self.assertEqual(kwargs["timeout"], 10)

    def test_network_failure_is_visible_and_never_becomes_demo_data(self):
        with patch("launcher.platform_api.build_opener") as opener:
            opener.return_value.open.side_effect = URLError("offline")
            with self.assertRaises(APIError):
                fetch_servers()

    def test_refuses_plain_http_and_cross_origin_redirects(self):
        with self.assertRaises(APIError):
            fetch_servers("http://hellboost.net/launcher?a=list")
        for url in ("http://hellboost.net/launcher", "https://other.example/launcher"):
            with self.subTest(url=url), self.assertRaises(APIError):
                SameOriginRedirect().redirect_request(Request("https://hellboost.net/launcher"), None, 302, "Found", {}, url)
