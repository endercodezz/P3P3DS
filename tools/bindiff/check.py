#!/usr/bin/env python3
"""Build/test the optional host tool; keep downloads and caches in the workspace."""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/bindiff"
VERSION = (TOOL / "zig-version").read_text().strip()
# SHA-256 from https://ziglang.org/download/index.json, release 0.17.1.
ARCHIVES = {
    "Windows": ("windows", "zip", "b03f477a1943673e02aca0f570e8030dffd536ce1f45cbc4d3326ec31159562e"),
    "Linux": ("linux", "tar.xz", "3581863e3cbcfd0fce0efc682ce7e9e93da96668a977791d356a26d2a7e9b17a"),
}


def download_zig():
    if platform.machine().lower() not in ("amd64", "x86_64"):
        raise RuntimeError("automatic download supports x86-64 Windows/Linux only; use --zig")
    system, ext, expected = ARCHIVES[platform.system()]
    cache = ROOT / ".cache/zig"
    cache.mkdir(parents=True, exist_ok=True)
    name = f"zig-x86_64-{system}-{VERSION}"
    archive = cache / f"{name}.{ext}"
    url = f"https://ziglang.org/download/{VERSION}/{archive.name}"
    if not archive.exists():
        partial = archive.with_suffix(archive.suffix + ".partial")
        with urllib.request.urlopen(url, timeout=120) as src, partial.open("wb") as dst:
            shutil.copyfileobj(src, dst)
        partial.replace(archive)
    with archive.open("rb") as src:
        actual = hashlib.file_digest(src, "sha256").hexdigest()
    if actual != expected:
        raise RuntimeError(f"Zig archive SHA-256 mismatch: {actual}")
    print(f"Verified {archive.name}: {actual}", flush=True)
    if ext == "zip":
        with zipfile.ZipFile(archive) as src:
            src.extractall(cache)
    else:
        with tarfile.open(archive) as src:
            src.extractall(cache, filter="data")
    return cache / name / ("zig.exe" if system == "windows" else "zig")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", default="zig", help="existing Zig 0.17.1 executable")
    parser.add_argument("--download", action="store_true", help="download and verify the pinned official compiler")
    args = parser.parse_args()
    zig = str(download_zig()) if args.download else args.zig
    env = os.environ.copy()
    for key, relative in {
        "ZIG_GLOBAL_CACHE_DIR": ".cache/zig-global",
        "ZIG_LOCAL_CACHE_DIR": ".cache/bindiff",
        "TEMP": ".tmp/bindiff",
        "TMP": ".tmp/bindiff",
        "TMPDIR": ".tmp/bindiff",
    }.items():
        path = ROOT / relative
        path.mkdir(parents=True, exist_ok=True)
        env[key] = str(path)

    def run(*command):
        print("+", *command, flush=True)
        subprocess.run(command, cwd=TOOL, env=env, check=True)

    version = subprocess.check_output([zig, "version"], env=env, text=True).strip()
    if version != VERSION:
        raise RuntimeError(f"expected Zig {VERSION}, got {version}")
    run(zig, "fmt", "--check", "build.zig", "src")
    run(zig, "build", "test", "-j4", "--summary", "all")
    run(zig, "build", "test", "-Doptimize=safe", "-j4", "--summary", "all")
    run(zig, "build", "-Doptimize=safe", "--prefix", str(ROOT / "build/bindiff"), "-j4", "--summary", "all")
    exe = ROOT / "build/bindiff/bin" / ("p3p-bindiff.exe" if os.name == "nt" else "p3p-bindiff")
    run(sys.executable, str(TOOL / "test_cli.py"), "--exe", str(exe))


if __name__ == "__main__":
    main()
