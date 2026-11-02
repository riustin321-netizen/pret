"""Read-only integration with the existing HellBoost launcher endpoint."""
import json
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener

try:
    from .runtime import validate_address
except ImportError:
    from runtime import validate_address

API_URL = "https://hellboost.net/launcher?a=list"
MAX_BODY = 2 * 1024 * 1024
MAX_SERVERS = 5000


class APIError(ValueError):
    pass


class SameOriginRedirect(HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, newurl):
        old, new = urlsplit(request.full_url), urlsplit(newurl)
        if (old.scheme, old.hostname, old.port) != (new.scheme, new.hostname, new.port):
            raise APIError("API-ul a redirecționat cererea către altă origine.")
        return super().redirect_request(request, fp, code, message, headers, newurl)


def parse_servers(body):
    if len(body) > MAX_BODY:
        raise APIError("Răspunsul API este prea mare.")
    try:
        document = json.loads(body)
    except (ValueError, UnicodeError) as exc:
        raise APIError("API-ul nu a returnat JSON valid.") from exc
    if not isinstance(document, dict) or not isinstance(document.get("servers"), list):
        raise APIError("Lipsește lista servers din răspuns.")
    if len(document["servers"]) > MAX_SERVERS:
        raise APIError("API-ul a returnat prea multe servere.")
    servers, seen = [], set()
    for item in document["servers"]:
        if not isinstance(item, dict):
            raise APIError("Înregistrare de server invalidă.")
        try:
            address = validate_address(item.get("address"))
        except ValueError as exc:
            raise APIError("API-ul a returnat o adresă invalidă.") from exc
        if address.lower() in seen:
            continue
        seen.add(address.lower())
        clean = {"address": address}
        for key in ("hostname", "map", "promo"):
            value = item.get(key, "")
            if value is None and key == "promo":
                value = ""
            if not isinstance(value, str) or len(value) > 512:
                raise APIError("Câmp text invalid în lista de servere.")
            clean[key] = "".join(c for c in value if c.isprintable())
        for key in ("players", "max_players"):
            value = item.get(key, 0)
            if type(value) is not int or not 0 <= value <= 255:
                raise APIError("Număr de jucători invalid.")
            clean[key] = value
        rank = item.get("rank", len(servers) + 1)
        if type(rank) is not int or rank < 1:
            raise APIError("Poziție invalidă în lista de servere.")
        clean["rank"] = rank
        servers.append(clean)
    return servers


def fetch_servers(url=API_URL):
    parsed = urlsplit(url)
    if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password:
        raise APIError("Lista serverelor necesită o adresă HTTPS fără credențiale.")
    request = Request(url, headers={"Accept": "application/json", "User-Agent": "HellBoost-Launcher/0.2"})
    try:
        with build_opener(SameOriginRedirect()).open(request, timeout=10) as response:
            body = response.read(MAX_BODY + 1)
    except (HTTPError, URLError, TimeoutError, OSError) as exc:
        raise APIError("Lista HellBoost nu a putut fi încărcată. Verifică rețeaua și endpointul API.") from exc
    return parse_servers(body)
