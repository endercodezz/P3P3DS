#!/usr/bin/env python3
"""Build/test the optional CPK tool; keep downloads and caches in the workspace."""
import argparse
import importlib.util
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/cpk"
VERSION = (TOOL / "zig-version").read_text().strip()


def download_zig():
    # One pinned compiler for both Zig tools: bindiff's helper holds the archive hashes.
    spec = importlib.util.spec_from_file_location("bindiff_check", ROOT / "tools/bindiff/check.py")
    bindiff = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bindiff)
    if bindiff.VERSION != VERSION:
        raise RuntimeError(f"tools/bindiff pins Zig {bindiff.VERSION}, tools/cpk pins {VERSION}")
    return bindiff.download_zig()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zig", default="zig", help="existing Zig 0.17.1 executable")
    parser.add_argument("--download", action="store_true", help="download and verify the pinned official compiler")
    args = parser.parse_args()
    zig = str(download_zig()) if args.download else args.zig
    env = os.environ.copy()
    for key, relative in {
        "ZIG_GLOBAL_CACHE_DIR": ".cache/zig-global",
        "ZIG_LOCAL_CACHE_DIR": ".cache/cpk",
        "TEMP": ".tmp/cpk",
        "TMP": ".tmp/cpk",
        "TMPDIR": ".tmp/cpk",
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
    run(zig, "build", "-Doptimize=safe", "--prefix", str(ROOT / "build/cpk"), "-j4", "--summary", "all")
    exe = ROOT / "build/cpk/bin" / ("p3p-cpk.exe" if os.name == "nt" else "p3p-cpk")
    run(sys.executable, str(TOOL / "test_cli.py"), "--exe", str(exe))


if __name__ == "__main__":
    main()
