#!/usr/bin/env python3
"""Run pspautotests programs through build/p3p_autotest and compare with a baseline.

Usage:
  python tools/run_autotests.py [--exe build/p3p_autotest.exe] [--jobs N] [--update] [glob ...]

Globs are relative to references/pspautotests/tests (default: cpu/*/*.prx
utility/systemparam/*.prx). The baseline tests/autotest_baseline.txt records,
per program, the number of output lines that differ from the hardware
`.expected` file and how the run stopped. --check (default) fails when any
program gets worse (more mismatching lines, or a previously clean exit now
stops abnormally); improvements are reported and pass, run --update to record
them. The baseline documents known gaps; it is never edited to hide a regression.
"""
import argparse
import concurrent.futures
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(ROOT, "references", "pspautotests", "tests")
BASELINE = os.path.join(ROOT, "tests", "autotest_baseline.txt")
LINE = re.compile(r'^(?P<name>\S+\.prx): stop="(?P<stop>[^"]*)" lines=(?P<lines>\d+) mismatches=(?P<mis>\d+)')


def run(exe, prx):
    rel = os.path.relpath(prx, TESTS).replace(os.sep, "/")
    try:
        out = subprocess.run([exe, prx], capture_output=True, text=True, errors="replace", timeout=900).stdout
    except subprocess.TimeoutExpired:
        return rel, ("timeout", 0, -1)
    for line in out.splitlines():
        m = LINE.match(line)
        if m:
            stop = m["stop"]
            kind = "exit" if stop in ("Program exited", "module_start returned to the harness") else "stop:" + stop
            return rel, (kind, int(m["lines"]), int(m["mis"]))
    return rel, ("no-summary", 0, -1)


def load_baseline():
    entries = {}
    if os.path.exists(BASELINE):
        for line in open(BASELINE, encoding="utf-8"):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            rel, mis, lines, kind = line.split("\t")
            entries[rel] = (kind, int(lines), int(mis))
    return entries


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(ROOT, "build", "p3p_autotest.exe" if os.name == "nt" else "p3p_autotest"))
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    ap.add_argument("--update", action="store_true")
    ap.add_argument("patterns", nargs="*", default=["cpu/*/*.prx", "utility/systemparam/*.prx"])
    args = ap.parse_args()
    prxs = sorted({p for pat in args.patterns for p in glob.glob(os.path.join(TESTS, pat))})
    if not prxs:
        print("no test programs found")
        return 1
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        results = dict(pool.map(lambda p: run(args.exe, p), prxs))
    baseline = load_baseline()
    worse, better, exact = [], [], 0
    for rel, (kind, lines, mis) in sorted(results.items()):
        clean = kind == "exit" and mis == 0 and lines > 0
        exact += clean
        tag = "EXACT" if clean else f"{mis}/{lines} differ, {kind}"
        old = baseline.get(rel)
        note = ""
        if old:
            okind, olines, omis = old
            if mis < 0 or mis > omis or (okind == "exit" and kind != "exit"):
                worse.append(rel); note = f"  WORSE (baseline {omis}/{olines}, {okind})"
            elif mis < omis or (okind != "exit" and kind == "exit"):
                better.append(rel); note = f"  better (baseline {omis}/{olines}, {okind})"
        else:
            note = "  (not in baseline)"
        print(f"{rel:<45} {tag}{note}")
    print(f"\n{exact}/{len(results)} programs match hardware exactly; worse={len(worse)} better={len(better)}")
    if args.update:
        with open(BASELINE, "w", encoding="utf-8", newline="\n") as f:
            f.write("# pspautotests on the P3P3DS interpreter (tools/run_autotests.py --update).\n")
            f.write("# program\tmismatching_lines\texpected_lines\tstop\n")
            for rel, (kind, lines, mis) in sorted(results.items()):
                f.write(f"{rel}\t{mis}\t{lines}\t{kind}\n")
        print(f"baseline written: {os.path.relpath(BASELINE, ROOT)}")
        return 0
    return 1 if worse else 0


if __name__ == "__main__":
    sys.exit(main())
