#!/usr/bin/env python3
"""Normalize runtime [HLE-TRACE] and PPSSPP log JSONL into canonical form.

Two input formats are supported. The canonical output form (RESEARCH §4.2) is::

    {seq:06d} {thread} {func_name} a0=0x{:08X} a1=0x{:08X} a2=0x{:08X} a3=0x{:08X} v0=0x{:08X}

Phase 11.5 Plan 01 / Wave 0. Companion to scripts/diff_traces.sh.

Usage:
    normalize_trace.py --input <path> --format <runtime|ppsspp> [--out <path>]
    normalize_trace.py --self-test
"""
import argparse
import json
import re
import sys

# ─── Parsers ──────────────────────────────────────────────────────────────────

# Runtime line 1 — `[HLE-TRACE] {thread}: {func}(a0=0x..., a1=0x..., a2=0x..., a3=0x...[ a0_str="..."])`
RT_CALL_RE = re.compile(
    r"\[HLE-TRACE\]\s+(?P<thread>[^:]+):\s+(?P<func>\S+?)"
    r"\(a0=0x(?P<a0>[0-9A-Fa-f]+),\s*a1=0x(?P<a1>[0-9A-Fa-f]+),"
    r"\s*a2=0x(?P<a2>[0-9A-Fa-f]+),\s*a3=0x(?P<a3>[0-9A-Fa-f]+)"
)

# Runtime line 2 — `[HLE-TRACE] {thread}: {func} -> v0=0x...`
RT_RET_RE = re.compile(
    r"\[HLE-TRACE\]\s+(?P<thread>[^:]+):\s+(?P<func>\S+)\s+->\s+v0=0x(?P<v0>[0-9A-Fa-f]+)"
)

# PPSSPP message body — `[PPSSPP-HLE] pc=... <func> a0=... a1=... a2=... a3=... v0=...`
# Tokens are space-separated; we anchor on key=value pairs.
PPSSPP_MSG_RE = re.compile(
    r"\[PPSSPP-HLE\]\s+pc=(?P<pc>[0-9A-Fa-f]+)\s+(?P<func>\S+)\s+"
    r"a0=(?P<a0>[0-9A-Fa-f]+)\s+a1=(?P<a1>[0-9A-Fa-f]+)\s+"
    r"a2=(?P<a2>[0-9A-Fa-f]+)\s+a3=(?P<a3>[0-9A-Fa-f]+)\s+"
    r"v0=(?P<v0>[0-9A-Fa-f]+)"
)


def _canonical(seq, thread, func, a0, a1, a2, a3, v0):
    """Format a canonical line. All hex strings are coerced to ints first."""
    return (
        f"{seq:06d} {thread} {func} "
        f"a0=0x{int(a0, 16):08X} a1=0x{int(a1, 16):08X} "
        f"a2=0x{int(a2, 16):08X} a3=0x{int(a3, 16):08X} "
        f"v0=0x{int(v0, 16):08X}"
    )


def parse_runtime(lines):
    """Yield canonical lines from a runtime stderr stream (2 lines per call).

    State machine: when a call line is matched, remember the (thread, func, a0..a3)
    triple; on the next matching return line whose (thread, func) align, emit
    one canonical line. Unpaired lines are dropped.
    """
    seq = 0
    pending = None  # (thread, func, a0, a1, a2, a3)
    for line in lines:
        m = RT_CALL_RE.search(line)
        if m is not None:
            pending = (
                m.group("thread"),
                m.group("func"),
                m.group("a0"),
                m.group("a1"),
                m.group("a2"),
                m.group("a3"),
            )
            continue
        m = RT_RET_RE.search(line)
        if m is not None and pending is not None:
            p_thread, p_func, p_a0, p_a1, p_a2, p_a3 = pending
            if p_thread == m.group("thread") and p_func == m.group("func"):
                seq += 1
                yield _canonical(
                    seq, p_thread, p_func, p_a0, p_a1, p_a2, p_a3, m.group("v0")
                )
            pending = None


def parse_ppsspp(lines):
    """Yield canonical lines from a PPSSPP JSONL log-event stream.

    Each input line is a JSON object. Frames with event="log" whose `message`
    starts with `[PPSSPP-HLE]` are decoded; everything else is silently dropped.
    """
    seq = 0
    for raw in lines:
        raw = raw.strip()
        if not raw:
            continue
        try:
            frame = json.loads(raw)
        except json.JSONDecodeError:
            continue
        if frame.get("event") != "log":
            continue
        msg = frame.get("message", "")
        if not isinstance(msg, str) or not msg.startswith("[PPSSPP-HLE]"):
            continue
        m = PPSSPP_MSG_RE.search(msg)
        if m is None:
            continue
        seq += 1
        # PPSSPP does not easily expose per-call thread name via the breakpoint
        # logFormat. Use a placeholder so the sequence-anchored diff still aligns
        # on (seq, func, a0..a3, v0) — thread name is filtered or ignored at diff time.
        yield _canonical(
            seq,
            "ppsspp",
            m.group("func"),
            m.group("a0"),
            m.group("a1"),
            m.group("a2"),
            m.group("a3"),
            m.group("v0"),
        )


# ─── Self-test ────────────────────────────────────────────────────────────────

RT_FIXTURE = [
    "[HLE-TRACE] boot: sceKernelLoadModule(a0=0x08DEAD00, a1=0x0, a2=0x0, a3=0x0)",
    "[HLE-TRACE] boot: sceKernelLoadModule -> v0=0x2A",
    "[HLE-TRACE] user_main: sceGeListEnQueue(a0=0x09000000, a1=0x09000400, a2=0x0, a3=0x0)",
    "[HLE-TRACE] user_main: sceGeListEnQueue -> v0=0x1",
]

PPSSPP_FIXTURE_JSONL = [
    json.dumps(
        {
            "event": "log",
            "timestamp": "00:00:01",
            "header": "Breakpoints.cpp:289",
            "message": (
                "[PPSSPP-HLE] pc=089D7608 sceKernelLoadModule "
                "a0=08DEAD00 a1=0 a2=0 a3=0 v0=2A"
            ),
            "level": 1,
            "channel": "G3D",
        }
    ),
    json.dumps(
        {
            "event": "log",
            "timestamp": "00:00:02",
            "header": "Breakpoints.cpp:289",
            "message": (
                "[PPSSPP-HLE] pc=089D78E8 sceGeListEnQueue "
                "a0=09000000 a1=09000400 a2=0 a3=0 v0=1"
            ),
            "level": 1,
            "channel": "G3D",
        }
    ),
]


def self_test():
    """Run an inline regression on the fixtures and print PASS/FAIL."""
    rt_out = list(parse_runtime(iter(RT_FIXTURE)))
    rt_expected = [
        "000001 boot sceKernelLoadModule a0=0x08DEAD00 a1=0x00000000 "
        "a2=0x00000000 a3=0x00000000 v0=0x0000002A",
        "000002 user_main sceGeListEnQueue a0=0x09000000 a1=0x09000400 "
        "a2=0x00000000 a3=0x00000000 v0=0x00000001",
    ]
    if rt_out != rt_expected:
        print(
            f"FAIL normalize_trace.py self-test (runtime parser):\n"
            f"  got:      {rt_out}\n"
            f"  expected: {rt_expected}",
            file=sys.stderr,
        )
        sys.exit(1)

    ppsspp_out = list(parse_ppsspp(iter(PPSSPP_FIXTURE_JSONL)))
    ppsspp_expected = [
        "000001 ppsspp sceKernelLoadModule a0=0x08DEAD00 a1=0x00000000 "
        "a2=0x00000000 a3=0x00000000 v0=0x0000002A",
        "000002 ppsspp sceGeListEnQueue a0=0x09000000 a1=0x09000400 "
        "a2=0x00000000 a3=0x00000000 v0=0x00000001",
    ]
    if ppsspp_out != ppsspp_expected:
        print(
            f"FAIL normalize_trace.py self-test (ppsspp parser):\n"
            f"  got:      {ppsspp_out}\n"
            f"  expected: {ppsspp_expected}",
            file=sys.stderr,
        )
        sys.exit(1)

    print("PASS normalize_trace.py self-test")
    sys.exit(0)


# ─── Main ─────────────────────────────────────────────────────────────────────


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--input", help="Path to input trace file")
    ap.add_argument(
        "--format",
        choices=["runtime", "ppsspp"],
        help="Input format (runtime stderr or ppsspp JSONL)",
    )
    ap.add_argument(
        "--out",
        help="Output path (default: stdout)",
        default=None,
    )
    ap.add_argument(
        "--self-test",
        action="store_true",
        help="Run the in-script regression on RT/PPSSPP fixtures and exit",
    )
    args = ap.parse_args()

    if args.self_test:
        self_test()
        return

    if args.input is None or args.format is None:
        ap.error("--input and --format are required unless --self-test is given")

    with open(args.input, "r", encoding="utf-8", errors="replace") as fh:
        lines = list(fh)

    if args.format == "runtime":
        gen = parse_runtime(iter(lines))
    else:
        gen = parse_ppsspp(iter(lines))

    out_fh = sys.stdout
    close_out = False
    if args.out is not None:
        out_fh = open(args.out, "w", encoding="utf-8")
        close_out = True
    try:
        for line in gen:
            out_fh.write(line)
            out_fh.write("\n")
    finally:
        if close_out:
            out_fh.close()


if __name__ == "__main__":
    main()
