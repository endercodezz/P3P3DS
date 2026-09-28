"""Bounded, execution-driven P3P frontier expansion. Standard library only.

Writes only a managed seed manifest and workspace build/report artifacts.
Never infers a caller from RA alone and never treats an unsafe stop as success.
By default, every accepted seed gets a full verification checkpoint. --fast
rebuilds and dual-replays each seed, then runs full verification every
--verify-every additions and at the final stop.
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
    if not isinstance(event, dict):
        raise ValueError("malformed event data: expected object")
    try:
        blocker, graphics, events = event["blocker"], event["graphics"], event["events"]
        if not isinstance(blocker, dict) or not isinstance(graphics, dict) or not isinstance(events, list):
            raise TypeError("blocker, graphics or events has wrong type")
        if not isinstance(blocker["type"], str):
            raise TypeError("blocker type has wrong type")
    except (KeyError, TypeError) as e:
        raise ValueError(f"malformed event data: {e}") from e
    if event.get("schema") != 1 or not event.get("bootstrap_passed"):
        return "bootstrap regression or unknown event schema"
    b = blocker
    if b["type"] != "missing_guest_function":
        return b["type"]
    try:
        if graphics["writer"] or graphics.get("color_writes", 0) or graphics.get("depth_writes", 0):
            return "graphics writer"
        if b["target"] != b["pc"] or b["word"] >> 26 != 3 or b["ra"] != b["caller"] + 8:
            return "unproven executed edge"
        edges = [e for e in events if e["type"] == "guest_transfer"]
        if not edges:
            return "missing executed edge"
        e = edges[-1]
        if (e["pc"], e["fields"]["word"], e["fields"]["target"], e["thread"]) != (b["caller"], b["word"], b["pc"], b["thread"]):
            return "stale executed edge"
    except (KeyError, TypeError, IndexError) as e:
        raise ValueError(f"malformed event data: {e}") from e
    return None


def blocker_details(event):
    blocker = dict(event["blocker"])
    for item in reversed(event["events"]):
        if item.get("type") == "missing_hle":
            fields = item.get("fields", {})
            if isinstance(item.get("detail"), str):
                blocker["module"] = item["detail"]
            for key in ("module", "nid"):
                if key in fields:
                    blocker[key] = fields[key]
            break
    return blocker


def parse_args(argv):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--build-dir", type=Path, default=ROOT / "build")
    p.add_argument("--output", type=Path, default=ROOT / ".tmp/frontier")
    p.add_argument("--max-additions", type=int, default=32)
    p.add_argument("--timeout", type=int, default=180)
    p.add_argument("--check-only", action="store_true")
    p.add_argument("--fast", action="store_true", help="opt in to periodic full verification")
    p.add_argument("--verify-every", type=int, default=10, metavar="N",
                   help="full checkpoints after N additions in --fast mode (default: 10)")
    a = p.parse_args(argv)
    if not 0 <= a.max_additions <= 128 or a.timeout <= 0 or a.verify_every <= 0:
        p.error("bounded limits required: additions 0..128, timeout > 0, verify-every > 0")
    return a


def chase_loop(ops, report, *, fast, max_additions, verify_every, check_only=False):
    """Advance only proven guest functions; replace ops with a fixture in workflow tests."""
    verified_manifest = ops.manifest()
    accepted = 0
    verified_additions = 0
    failing_candidate = None
    report.update(mode="fast" if fast else "safe", starting_seed_count=ops.seed_count(),
                  accepted_candidates=[], verification_checkpoints=[], steps=[],
                  last_known_good_seed_count=ops.seed_count(), verified_additions=0,
                  tentative_additions=0, rollback=None, final_verification="NOT_RUN")

    def checkpoint():
        nonlocal verified_manifest, verified_additions
        observation = ops.full_verify(accepted)
        reason = candidate(observation["event"])
        if reason == "bootstrap regression or unknown event schema":
            raise RuntimeError(reason)
        verified_manifest = ops.manifest()
        verified_additions = accepted
        report["verified_additions"] = accepted
        report["tentative_additions"] = 0
        report["last_known_good_seed_count"] = ops.seed_count()
        seeds = ops.managed_seeds()
        report["last_verified_seed"] = seeds[-1] if seeds else None
        report["verification_checkpoints"].append({
            "accepted_additions": accepted, "seed_count": ops.seed_count(),
            "events_sha256": observation["hashes"][0], "blocker": observation["event"]["blocker"]})
        return observation

    def finish(reason, observation):
        if accepted != verified_additions:
            before = observation["hashes"][0]
            observation = checkpoint()  # Final full verification is mandatory.
            if observation["hashes"][0] != before:
                raise RuntimeError("final checkpoint changed runtime events")
        report["stop"] = reason
        report["final_replay_hashes"] = observation["hashes"]
        report["final_blocker"] = blocker_details(observation["event"])
        report["final_verification"] = "PASS"

    try:
        observation = checkpoint()  # Fully verified starting point.
        while True:
            events = observation["event"]
            reason = candidate(events)
            b = events["blocker"]
            step = {"blocker": b, "graphics": events["graphics"],
                    "events_sha256": observation["hashes"][0], "proof_result": reason}
            report["steps"].append(step)
            if reason:
                finish(reason, observation)
                break
            failing_candidate = {"pc": b["pc"], "caller": b["caller"]}
            proof = ops.prove(b)
            step["proof"] = proof
            step["proof_result"] = "accepted" if proof["accepted"] else proof["reason"]
            if not proof["accepted"]:
                finish("ambiguous_boundary: " + proof["reason"], observation)
                break
            if check_only or accepted >= max_additions:
                finish("check_only" if check_only else "addition_budget", observation)
                break
            ops.append_seed(b)
            accepted += 1
            report["accepted_candidates"].append({**failing_candidate, "proof": proof})
            report["tentative_additions"] = accepted - verified_additions
            if not fast or accepted - verified_additions >= verify_every:
                observation = checkpoint()
            else:
                ops.build()  # AOT must be rebuilt for every accepted seed.
                observation = ops.capture(accepted)
                reason = candidate(observation["event"])
                if reason == "bootstrap regression or unknown event schema":
                    raise RuntimeError(reason)
            failing_candidate = None
    except (Exception, KeyboardInterrupt) as e:
        report["error"] = True
        report["stop"] = "workflow_error: " + str(e)
        try:
            tentative = max(0, ops.seed_count() - report["last_known_good_seed_count"])
        except Exception:
            tentative = accepted - verified_additions
        report["rollback"] = {"tentative_additions": tentative,
                              "first_failing_candidate": failing_candidate,
                              "last_verified_frontier": report["verification_checkpoints"][-1]["blocker"]
                              if report["verification_checkpoints"] else None,
                              "manifest_restored": False, "rebuild_succeeded": False}
        try:
            if ops.manifest() != verified_manifest:
                ops.restore(verified_manifest)
                ops.build()  # Restore generated AOT and executable to the checkpoint.
            report["rollback"]["manifest_restored"] = ops.manifest() == verified_manifest
            report["rollback"]["rebuild_succeeded"] = True
            report["tentative_additions"] = 0
        except Exception as rollback_error:
            report["rollback"]["error"] = str(rollback_error)
    try:
        report["managed_seeds"] = ops.managed_seeds()
    except Exception as e:
        report["error"] = True
        report["managed_seeds_error"] = str(e)
    return report


class WorkspaceOps:
    def __init__(self, build, output, timeout, env):
        self.build_dir, self.output, self.timeout, self.env = build, output, timeout, env
        self.command_index = 0
        self.capture_index = 0
        suffix = ".exe" if os.name == "nt" else ""
        self.bootstrap = build / ("p3p_pc_bootstrap" + suffix)
        self.checker = build / ("p3p_frontier_check" + suffix)
        self.generator = build / "recomp/PSPRecomp" / ("psp_recomp" + suffix)

    def run(self, command, label, allowed=(0,)):
        self.command_index += 1
        log = self.output / f"command_{self.command_index:03d}_{label}.log"
        with log.open("w", encoding="utf-8") as f:
            f.write(json.dumps([str(x) for x in command]) + "\n")
            f.flush()
            result = subprocess.run([str(x) for x in command], cwd=ROOT, env=self.env,
                                    stdout=f, stderr=subprocess.STDOUT, timeout=self.timeout)
        if result.returncode not in allowed:
            raise RuntimeError(f"{label} exit {result.returncode}; see {log.relative_to(ROOT)}")
        return log

    def manifest(self):
        return SEEDS.read_bytes()

    def restore(self, content):
        SEEDS.write_bytes(content)

    def managed_seeds(self):
        return rows(SEEDS)

    def seed_count(self):
        return len(rows(SEEDS))

    def append_seed(self, blocker):
        managed = rows(SEEDS)
        managed.append({"name": f"sub_{blocker['pc']:08X}",
                        "address": f"0x{blocker['pc']:08X}", "size": "cfg"})
        SEEDS.write_bytes(encoded(managed))
        print(f"[{len(managed)} seeds] proven 0x{blocker['pc']:08X}; regenerate/build", flush=True)

    def build(self):
        self.run(["cmake", "--build", self.build_dir, "--config", "Release", "--parallel", "2"], "build")

    def capture(self, additions):
        paths = []
        self.capture_index += 1
        for repeat in range(2):
            path = self.output / f"events_{self.capture_index:03d}_{additions:03d}_{repeat}.json"
            self.run([self.bootstrap, "--run-until-blocker", "--dump-events", path],
                     f"run_{additions}_{repeat}", allowed=(0, 3))
            paths.append(path.read_bytes())
        if paths[0] != paths[1]:
            raise RuntimeError("non-deterministic runtime events")
        digest = hashlib.sha256(paths[0]).hexdigest()
        return {"event": json.loads(paths[0]), "hashes": [digest, digest]}

    def full_verify(self, additions):
        self.build()
        self.run(["ctest", "--test-dir", self.build_dir, "--output-on-failure"], "ctest")
        self.run([self.bootstrap, "--verify-bootstrap"], "bootstrap")
        observation = self.capture(additions)
        repeat_cpp = self.output / "regenerated.cpp"
        self.run([self.generator, ELF, self.build_dir / "generated/frontier_functions.csv", repeat_cpp], "regenerate")
        if repeat_cpp.read_bytes() != (self.build_dir / "generated/p3p_generated.cpp").read_bytes():
            raise RuntimeError("non-deterministic generated source")
        return observation

    def prove(self, blocker):
        log = self.run([self.checker, ELF, self.build_dir / "generated/frontier_functions.csv",
                        hex(blocker["pc"]), hex(blocker["caller"]), hex(blocker["word"])],
                       "proof", allowed=(0, 2))
        proof = json.loads(log.read_text(encoding="utf-8").splitlines()[-1])
        if not isinstance(proof, dict) or not isinstance(proof.get("accepted"), bool) or not isinstance(proof.get("reason"), str):
            raise ValueError("malformed validator proof")
        return proof


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if argv and argv[0] == "--merge":
        merge(*argv[1:])
        return 0
    a = parse_args(argv)
    build, output = contained(a.build_dir), contained(a.output)
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TEMP=str(ROOT / ".tmp"), TMP=str(ROOT / ".tmp"), TMPDIR=str(ROOT / ".tmp"), PYTHONDONTWRITEBYTECODE="1")
    report = {"schema": 1, "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "elf_sha256": hashlib.sha256(ELF.read_bytes()).hexdigest()}
    ops = WorkspaceOps(build, output, a.timeout, env)
    report = chase_loop(ops, report, fast=a.fast, max_additions=a.max_additions,
                        verify_every=a.verify_every, check_only=a.check_only)
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Auto-recompile complete")
    for label, key in (("Mode", "mode"), ("Starting seeds", "starting_seed_count"),
                       ("Accepted functions", "accepted_candidates"), ("Verified additions", "verified_additions"),
                       ("Tentative additions", "tentative_additions"),
                       ("Final verification", "final_verification"), ("Stop reason", "stop")):
        value = len(report[key]) if key == "accepted_candidates" else report[key]
        print(f"{label:21}{value}")
    blocker = report.get("final_blocker") or {}
    if blocker:
        print(f"PC:                  0x{blocker['pc']:08X}")
        print(f"Caller:              0x{blocker['caller']:08X}")
        print(f"Module:              {blocker.get('module', 'unavailable')}")
        if "nid" in blocker:
            print(f"NID:                 0x{blocker['nid']:08X}")
    print(f"Event SHA-256:       {report.get('final_replay_hashes', ['unavailable'])[0]}")
    print(f"Last verified seeds: {report['last_known_good_seed_count']}")
    if report.get("last_verified_seed"):
        print(f"Last verified seed:  {report['last_verified_seed']['address']}")
    if report["rollback"]:
        print(f"Rollback:            {report['rollback']}")
    print(output / "report.json")
    return 1 if report.get("error") else 0


if __name__ == "__main__":
    raise SystemExit(main())
