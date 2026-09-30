"""Fail if `psp_recomp --auto` output disagrees with the configured unit layout.

Usage: verify_aot_layout.py <generated_dir> <unit_count> <unit_span>
Standard library only.
"""
import json
from pathlib import Path
import sys


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    out, count, span = Path(argv[0]), int(argv[1]), int(argv[2])
    report = json.loads((out / "auto_codegen_report.json").read_text(encoding="utf-8"))
    errors = []
    if report.get("translation_units") != count:
        errors.append(f"translation_units={report.get('translation_units')} expected {count}")
    if report.get("unit_span_bytes") != span:
        errors.append(f"unit_span_bytes={report.get('unit_span_bytes')} expected {span}")
    expected = {f"generated_unit_{i:04d}.cpp" for i in range(count)} | {"generated_registry.cpp"}
    present = {p.name for p in out.glob("generated_*.cpp")}
    if missing := sorted(expected - present):
        errors.append(f"missing units: {missing[:5]}")
    if extra := sorted(present - expected):
        errors.append(f"unexpected units: {extra[:5]}")
    if errors:
        print("AOT layout mismatch (update profiles/p3p/config/aot_layout.cmake after review): "
              + "; ".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
