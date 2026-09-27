"""Bounded, execution-driven P3P frontier expansion. Standard library only.

Writes only a managed seed manifest and workspace build/report artifacts.
Never infers a caller from RA alone and never treats an unsafe stop as success.
"""
import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "profiles/p3p/config/p3p_functions.csv"
SEEDS = ROOT / "profiles/p3p/config/frontier_seeds.csv"
ELF = ROOT / "profiles/p3p/game/eboot.elf"


def contained(path):
    path = Path(path).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"path outside workspace: {path}")
    return path


def rows(path):
    with Path(path).open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != ["name", "address", "size"]:
            raise ValueError("invalid seed manifest header")
        result = list(reader)
    for row in result:
        if set(row) != {"name", "address", "size"} or not all(row.values()):
            raise ValueError("invalid seed row")
        address = int(row["address"], 16)
        if address & 3 or not 0 <= address <= 0xFFFFFFFF:
            raise ValueError("invalid seed address")
    return result


def encoded(entries):
    s = io.StringIO(newline="")
    w = csv.DictWriter(s, fieldnames=["name", "address", "size"], lineterminator="\n")
    w.writeheader()
    w.writerows(entries)
    return s.getvalue().encode()


def merge(base, managed, output):
    entries = rows(base) + rows(managed)
    addresses = [int(r["address"], 16) for r in entries]
    if len(set(addresses)) != len(addresses):
        raise ValueError("duplicate seed address")
    contained(output).write_bytes(encoded(entries))


def candidate(event):
    if event.get("schema") != 1 or not event.get("bootstrap_passed"):
        return "bootstrap regression or unknown event schema"
    b = event["blocker"]
    if b["type"] != "missing_guest_function":
        return b["type"]
    if event["graphics"]["writer"] or event["graphics"].get("color_writes", 0) or event["graphics"].get("depth_writes", 0):
        return "graphics writer"
    if b["target"] != b["pc"] or b["word"] >> 26 != 3 or b["ra"] != b["caller"] + 8:
        return "unproven executed edge"
    edges = [e for e in event["events"] if e["type"] == "guest_transfer"]
    if not edges:
        return "missing executed edge"
    e = edges[-1]
    if (e["pc"], e["fields"]["word"], e["fields"]["target"], e["thread"]) != (b["caller"], b["word"], b["pc"], b["thread"]):
        return "stale executed edge"
    return None


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if argv and argv[0] == "--merge":
        merge(*argv[1:])
        return 0
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--build-dir", type=Path, default=ROOT / "build")
    p.add_argument("--output", type=Path, default=ROOT / ".tmp/frontier")
    p.add_argument("--max-additions", type=int, default=32)
    p.add_argument("--timeout", type=int, default=180)
    p.add_argument("--check-only", action="store_true")
    a = p.parse_args(argv)
    if not 0 <= a.max_additions <= 128 or a.timeout <= 0:
        p.error("bounded limits required: additions 0..128, timeout > 0")
    build, output = contained(a.build_dir), contained(a.output)
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TEMP=str(ROOT / ".tmp"), TMP=str(ROOT / ".tmp"), TMPDIR=str(ROOT / ".tmp"), PYTHONDONTWRITEBYTECODE="1")
    suffix = ".exe" if os.name == "nt" else ""
    bootstrap = build / ("p3p_pc_bootstrap" + suffix)
    checker = build / ("p3p_frontier_check" + suffix)
    generator = build / "recomp/PSPRecomp" / ("psp_recomp" + suffix)
    report = {"schema": 1, "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "elf_sha256": hashlib.sha256(ELF.read_bytes()).hexdigest(), "steps": []}
    command_index = 0

    def run(command, label, allowed=(0,)):
        nonlocal command_index
        command_index += 1
        log = output / f"command_{command_index:03d}_{label}.log"
        with log.open("w", encoding="utf-8") as f:
            f.write(json.dumps([str(x) for x in command]) + "\n")
            f.flush()
            result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=a.timeout)
        if result.returncode not in allowed:
            raise RuntimeError(f"{label} exit {result.returncode}; see {log.relative_to(ROOT)}")
        return log

    def build_and_test():
        run(["cmake", "--build", build, "--config", "Release", "--parallel", "2"], "build")
        run(["ctest", "--test-dir", build, "--output-on-failure"], "ctest")

    previous_manifest = None
    try:
        build_and_test()
        for index in range(a.max_additions + 1):
            snapshots = []
            for repeat in range(2):
                path = output / f"events_{index:03d}_{repeat}.json"
                run([bootstrap, "--run-until-blocker", "--dump-events", path], f"run_{index}_{repeat}", allowed=(0, 3))
                snapshots.append(path.read_bytes())
            if snapshots[0] != snapshots[1]:
                raise RuntimeError("non-deterministic runtime events")
            previous_manifest = None  # New seed has now passed build/tests/two replays.
            events = json.loads(snapshots[0])
            step = {"blocker": events["blocker"], "graphics": events["graphics"],
                    "events_sha256": hashlib.sha256(snapshots[0]).hexdigest()}
            report["steps"].append(step)
            reason = candidate(events)
            if reason:
                report["stop"] = reason
                break
            b = events["blocker"]
            log = run([checker, ELF, build / "generated/frontier_functions.csv", hex(b["pc"]), hex(b["caller"]), hex(b["word"])], "proof", allowed=(0, 2))
            proof = json.loads(log.read_text(encoding="utf-8").splitlines()[-1])
            step["proof"] = proof
            if not proof["accepted"]:
                report["stop"] = "ambiguous_boundary: " + proof["reason"]
                break
            if a.check_only or index == a.max_additions:
                report["stop"] = "check_only" if a.check_only else "addition_budget"
                break
            managed = rows(SEEDS)
            managed.append({"name": f"sub_{b['pc']:08X}", "address": f"0x{b['pc']:08X}", "size": "cfg"})
            previous_manifest = SEEDS.read_bytes()
            SEEDS.write_bytes(encoded(managed))
            print(f"[{index+1}] proven 0x{b['pc']:08X}; regenerate/build/test", flush=True)
            build_and_test()
            repeat_cpp = output / "regenerated.cpp"
            run([generator, ELF, build / "generated/frontier_functions.csv", repeat_cpp], "regenerate")
            if repeat_cpp.read_bytes() != (build / "generated/p3p_generated.cpp").read_bytes():
                raise RuntimeError("non-deterministic generated source")
        report["managed_seeds"] = rows(SEEDS)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as e:
        report["stop"] = "workflow_error: " + str(e)
        if previous_manifest is not None:
            SEEDS.write_bytes(previous_manifest)
            report["rollback"] = "manifest restored; rebuild required before next run"
        report["error"] = True
    finally:
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report["stop"])
    print(output / "report.json")
    return 1 if report.get("error") else 0


if __name__ == "__main__":
    raise SystemExit(main())
