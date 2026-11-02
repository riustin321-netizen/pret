"""Create development packages from an explicit source allowlist, never game files."""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parent.parent


def require_pe32(path):
    data = path.read_bytes()
    if len(data) < 64 or data[:2] != b"MZ":
        raise SystemExit(f"Not a Windows executable: {path.name}")
    offset = struct.unpack_from("<I", data, 60)[0]
    if offset + 26 > len(data) or data[offset:offset + 4] != b"PE\0\0":
        raise SystemExit(f"Invalid Windows executable: {path.name}")
    machine = struct.unpack_from("<H", data, offset + 4)[0]
    optional_magic = struct.unpack_from("<H", data, offset + 24)[0]
    if machine != 0x14C or optional_magic != 0x10B:
        raise SystemExit(f"Expected Windows x86 / PE32: {path.name}")


def package(windows=False, native=False):
    files = []
    for directory in ("launcher", "native", "tests", "docs", "tools", ".github/workflows", "third_party"):
        files.extend(p for p in (ROOT / directory).rglob("*") if p.is_file() and p.suffix in (".py", ".cpp", ".h", ".hpp", ".def", ".md", ".yml", ".txt", ".ps1", ".rc", ".manifest", ".MIT"))
    files.extend(ROOT / name for name in ("README.md", "PORNESTE.cmd", "build-native.cmd", ".gitignore"))
    binaries = []
    if windows:
        binaries.append((ROOT / "build/HellBoost.exe", "HellBoost.exe"))
    if windows or native:
        binaries.extend([(ROOT / "build/HellBoostClient.dll", "native/HellBoostClient.dll"), (ROOT / "build/abi_test.exe", "native/abi_test.exe"), (ROOT / "build/browser_test.exe", "native/browser_test.exe")])
    if windows or native:
        for path, _ in binaries:
            if not path.is_file():
                raise SystemExit(f"Missing Windows build: {path.name}. No Windows package was produced.")
            require_pe32(path)
    name = "HellBoost-dev-windows-x86.zip" if windows else "HellBoost-native-dev-x86.zip" if native else "HellBoost-dev-source.zip"
    output = ROOT / "dist" / name
    output.parent.mkdir(parents=True, exist_ok=True)
    entries = [(path, "HellBoost/" + path.relative_to(ROOT).as_posix()) for path in sorted(files)]
    entries.extend((path, "HellBoost/" + name) for path, name in binaries)
    manifest = []
    with ZipFile(output, "w", ZIP_DEFLATED) as archive:
        for path, name in entries:
            content = path.read_bytes()
            archive.writestr(name, content)
            manifest.append({"path": name, "bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()})
        archive.writestr("HellBoost/manifest.json", json.dumps({"development_build": True, "files": manifest}, indent=2))
    with ZipFile(output) as archive:
        if archive.testzip() is not None:
            raise SystemExit("Archive validation failed")
    print(output)
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--windows", action="store_true", help="Require compiled launcher EXE, x86 DLL and ABI test EXE")
    group.add_argument("--native", action="store_true", help="Include x86 DLL and ABI test EXE, Python launcher remains source")
    args = parser.parse_args()
    try:
        package(args.windows, args.native)
    except (Exception, SystemExit) as error:
        if os.environ.get("GITHUB_ACTIONS") == "true":
            detail = str(error).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
            print(f"::error::Packaging failed: {detail}", flush=True)
        raise
