#!/usr/bin/env python3
"""Summarize a p3p_pc_bootstrap --dump-events JSON.

Usage: summarize_events.py <events.json> [--types substr,substr] [--last N]

Prints the blocker, the stop reason, per-type event counts (the dump keeps
only the first occurrences of capped types; counts are complete), frame and
audio summaries, missing HLE imports and the last N stored events.
"""
import json
import sys


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = sys.argv[1]
    types = []
    last = 8
    args = sys.argv[2:]
    for i, a in enumerate(args):
        if a == "--types" and i + 1 < len(args):
            types = [t for t in args[i + 1].split(",") if t]
        if a == "--last" and i + 1 < len(args):
            last = int(args[i + 1])
    with open(path, encoding="utf-8") as f:
        d = json.load(f)
    b = d.get("blocker", {})
    print(f"blocker: {b.get('type')}  pc=0x{b.get('pc', 0):08X}  thread={b.get('thread')}")
    print(f"reason:  {b.get('reason')}")
    print(f"last transfer: 0x{b.get('caller', 0):08X} -> 0x{b.get('target', 0):08X} word=0x{b.get('word', 0):08X}")
    print(f"bootstrap_passed: {d.get('bootstrap_passed')}")
    counts = d.get("event_type_counts") or {}
    shown = sorted(counts.items(), key=lambda kv: -kv[1])
    if types:
        shown = [kv for kv in shown if any(t in kv[0] for t in types)]
    print("event counts:", ", ".join(f"{k}={v}" for k, v in shown[:40]))
    events = d.get("events", [])
    frames = [e for e in events if e["type"] == "frame"]
    if frames:
        f0 = frames[-1]["fields"]
        lit = sum(1 for e in frames if e["fields"].get("nonblack_pixels", 0))
        print(f"frames: {len(frames)} (non-black {lit}); last index={f0['index']} "
              f"nonblack={f0['nonblack_pixels']} t={f0['time_us'] / 1e6:.1f}s fnv1a={f0['fnv1a']:016X}")
    for e in events:
        if e["type"] in ("audio_summary", "interpreter_summary"):
            print(e["type"] + ":", e["fields"])
        if e["type"] == "missing_hle":
            print(f"missing_hle: {e['detail']} nid=0x{e['fields']['nid']:08X} stub=0x{e['fields']['stub']:08X}")
    print(f"last {last} events:")
    for e in events[-last:]:
        fields = {k: (hex(v) if isinstance(v, int) and v > 0xFFFF else v) for k, v in e["fields"].items()}
        print(f"  {e['type']:<24} pc=0x{e['pc']:08X} thr={e['thread']} {e.get('detail', '')} {fields}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
