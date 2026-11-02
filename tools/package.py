"""Create development packages from an explicit source allowlist, never game files."""
import argparse
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parent.parent


def package(windows=False, native=False):
    files = []
    for directory in ("launcher", "native", "tests", "docs", "tools", "integrations", ".github/workflows", "third_party"):
        files.extend(p for p in (ROOT / directory).rglob("*") if p.is_file() and p.suffix in (".py", ".cpp", ".h", ".hpp", ".def", ".md", ".php", ".vdf", ".yml", ".txt"))
    files.extend(ROOT / name for name in ("README.md", "PORNESTE.cmd", "build-native.cmd", ".gitignore"))
    binaries = []
    if windows:
        binaries.append((ROOT / "dist/HellBoost.exe", "HellBoost.exe"))
    if windows or native:
        binaries.extend([(ROOT / "build/HellBoostClient.dll", "native/HellBoostClient.dll"), (ROOT / "build/abi_test.exe", "native/abi_test.exe")])
    if windows or native:
        for path, _ in binaries:
            if not path.is_file():
                raise SystemExit(f"Missing Windows build: {path.name}. No Windows package was produced.")
    name = "HellBoost-dev-windows-x86.zip" if windows else "HellBoost-native-dev-x86.zip" if native else "HellBoost-dev-source.zip"
    output = ROOT / "dist" / name
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, "w", ZIP_DEFLATED) as archive:
        for path in sorted(files):
            archive.write(path, "HellBoost/" + path.relative_to(ROOT).as_posix())
        if windows or native:
            for path, name in binaries:
                archive.write(path, "HellBoost/" + name)
        manifest = []
        for name in archive.namelist():
            content = archive.read(name)
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
    package(args.windows, args.native)
