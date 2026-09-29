import importlib.util
import sys
from pathlib import Path
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("chase", ROOT / "tools/chase_frontier.py")
chase = importlib.util.module_from_spec(spec)
spec.loader.exec_module(chase)


class ChaserTests(unittest.TestCase):
    def event(self):
        return {"schema": 1, "bootstrap_passed": True,
                "blocker": {"type": "missing_guest_function", "proof_kind": "direct_jal", "pc": 4096, "caller": 512, "word": 0x0C000400, "target": 4096, "ra": 520, "thread": 2},
                "graphics": {"writer": False, "vram_operations": 0},
                "events": [{"type": "guest_transfer", "pc": 512, "thread": 2, "fields": {"word": 0x0C000400, "target": 4096}}]}

    def thread_event(self):
        e = self.event()
        e["blocker"].update(pc=0x1000, thread=5, proof_kind="thread_entry",
                            provenance={"kind": "thread_entry", "uid": 5, "entry": 0x1000, "from_uid": 2})
        e["events"] = [
            {"type": "thread_create", "thread": 2, "detail": "worker", "fields": {"uid": 5, "entry": 0x1000}},
            {"type": "thread_start", "thread": 5, "fields": {"uid": 5, "result": 0}},
            {"type": "thread_switch", "thread": 5, "fields": {"uid": 5, "entry": 0x1000}},
            {"type": "thread_entry_transfer", "thread": 5, "detail": "worker",
             "fields": {"uid": 5, "entry": 0x1000, "from_uid": 2, "started": 1}},
            {"type": "stop", "thread": 5, "fields": {"target": 0x1000}},
        ]
        return e

    def test_stop_classes(self):
        e = self.event()
        self.assertIsNone(chase.candidate(e))
        for kind in ("graphics_writer", "cpu_vram_write", "missing_hle", "unsupported_instruction", "memory_fault", "unproven_transfer", "runtime_semantics", "budget_exhausted"):
            e["blocker"]["type"] = kind
            self.assertEqual(chase.candidate(e), kind)

    def test_stale_caller_and_regression(self):
        e = self.event(); e["events"][-1]["thread"] = 3
        self.assertEqual(chase.candidate(e), "stale executed edge")
        e = self.event(); e["bootstrap_passed"] = False
        self.assertIsNotNone(chase.candidate(e))
        e = self.event(); e["graphics"]["vram_operations"] = 1
        self.assertIsNone(chase.candidate(e))  # metadata is not rendering
        e["graphics"]["color_writes"] = 1
        self.assertEqual(chase.candidate(e), "graphics writer")

    def test_thread_entry_provenance(self):
        e = self.thread_event()
        self.assertIsNone(chase.candidate(e))
        e["blocker"]["thread"] = 6
        self.assertEqual(chase.candidate(e), "mismatched thread-entry provenance")
        e = self.thread_event(); e["blocker"]["pc"] = 0x1004
        self.assertEqual(chase.candidate(e), "mismatched thread-entry provenance")
        e = self.thread_event(); e["events"].pop(1)
        self.assertEqual(chase.candidate(e), "missing successful thread start")
        e = self.thread_event(); e["events"].insert(-1,{"type": "guest_enter", "thread": 5, "fields": {"target": 0x1000}})
        self.assertEqual(chase.candidate(e), "stale thread-entry transfer")
        e = self.thread_event(); e["events"].pop(2)
        self.assertEqual(chase.candidate(e), "missing matching thread switch")
        e = self.thread_event(); e["events"].pop(3)
        self.assertEqual(chase.candidate(e), "missing fresh thread-entry transfer")
        e = self.thread_event(); e["blocker"].pop("provenance")
        with self.assertRaises(ValueError): chase.candidate(e)
        e = self.thread_event(); e["blocker"].pop("proof_kind")
        with self.assertRaises(ValueError): chase.candidate(e)

    def test_manifest_duplicate_and_containment(self):
        with tempfile.TemporaryDirectory(dir=ROOT / ".tmp") as d:
            a, b, out = [Path(d) / name for name in ("a.csv", "b.csv", "out.csv")]
            a.write_bytes(chase.encoded([{"name": "a", "address": "0x1000", "size": "cfg"}]))
            b.write_bytes(chase.encoded([]));chase.merge(a, b, out)
            self.assertEqual(a.read_bytes(), out.read_bytes())
            b.write_bytes(a.read_bytes())
            with self.assertRaises(ValueError):chase.merge(a, b, out)
        with self.assertRaises(ValueError):chase.contained(ROOT.parent / "escape")

    def test_invalid_cadence(self):
        for value in ("0", "-1"):
            with self.subTest(value=value), self.assertRaises(SystemExit):
                chase.parse_args(["--verify-every", value])


class FakeOps:
    def __init__(self, chain, *, fail_at=None, ambiguous_at=None, bad_at=None):
        self.chain, self.index = chain, 0
        self.fail_at, self.ambiguous_at, self.bad_at = fail_at, ambiguous_at, bad_at
        self.full_calls, self.build_calls, self.proof_calls = [], [], []
        self.restored = False

    def manifest(self):
        return str(self.index).encode()

    def restore(self, content):
        self.index = int(content)
        self.restored = True

    def managed_seeds(self):
        return [{"name": str(i)} for i in range(self.index)]

    def seed_count(self):
        return 18 + self.index

    def append_seed(self, blocker):
        self.index += 1

    def build(self):
        self.build_calls.append(self.index)

    def event(self):
        pc = 0x1000 + self.index * 0x100
        caller = 0x200 + self.index * 8
        word = 0x0C000000 | (pc >> 2)
        if self.index >= self.chain:
            return {"schema": 1, "bootstrap_passed": True,
                    "blocker": {"type": "missing_hle", "pc": pc, "caller": caller},
                    "graphics": {"writer": False},
                    "events": [{"type": "missing_hle", "detail": "sceUmdUser",
                                "fields": {"nid": 0x8EF08FCE}}]}
        return {"schema": 1, "bootstrap_passed": True,
                "blocker": {"type": "missing_guest_function", "proof_kind": "direct_jal", "pc": pc,
                            "caller": caller, "word": word, "target": pc,
                            "ra": caller + 8, "thread": 2},
                "graphics": {"writer": False},
                "events": [{"type": "guest_transfer", "pc": caller, "thread": 2,
                            "fields": {"word": word, "target": pc}}]}

    def capture(self, additions):
        event = self.event()
        if self.bad_at == self.index:
            event = {"schema": 1, "bootstrap_passed": True, "blocker": []}
        return {"event": event, "hashes": [f"hash-{self.index}"] * 2}

    def full_verify(self, additions):
        self.full_calls.append(self.index)
        if self.fail_at == self.index:
            raise RuntimeError("fixture verification failure")
        return self.capture(additions)

    def prove(self, blocker):
        self.proof_calls.append(self.index)
        if self.ambiguous_at == self.index:
            return {"accepted": False, "reason": "ambiguous target"}
        return {"accepted": True, "proof_kind": blocker["proof_kind"], "reason": "closed CFG"}


class ThreadOps(FakeOps):
    def event(self):
        if self.index == 0:
            e = ChaserTests().thread_event()
            return e
        return super().event()


class MalformedThreadOps(ThreadOps):
    def event(self):
        e = super().event()
        if self.index == 0:
            e["blocker"].pop("provenance")
        return e


class WorkflowTests(unittest.TestCase):
    def run_chase(self, ops, *, fast, cadence=2, budget=128, check_only=False):
        return chase.chase_loop(ops, {}, fast=fast, max_additions=budget,
                                verify_every=cadence, check_only=check_only)

    def test_safe_fully_verifies_every_addition(self):
        ops = FakeOps(4)
        report = self.run_chase(ops, fast=False)
        self.assertEqual(ops.full_calls, [0, 1, 2, 3, 4])
        self.assertEqual(report["verified_additions"], 4)
        self.assertEqual(report["final_verification"], "PASS")
        self.assertEqual(report["stop"], "missing_hle")

    def test_thread_entry_seed_workflow(self):
        ops = ThreadOps(1)
        report = self.run_chase(ops, fast=True)
        self.assertEqual(ops.index, 1)
        self.assertEqual(ops.proof_calls, [0])
        self.assertEqual(report["accepted_candidates"][0]["proof_kind"], "thread_entry")
        self.assertEqual(report["final_verification"], "PASS")
        self.assertEqual(report["stop"], "missing_hle")

    def test_missing_thread_provenance_never_adds_seed(self):
        ops = MalformedThreadOps(1)
        report = self.run_chase(ops, fast=True)
        self.assertEqual(ops.index, 0)
        self.assertTrue(report["error"])
        self.assertEqual(report["verified_additions"], 0)
        self.assertEqual(ops.proof_calls, [])

    def test_fast_cadence_and_final_checkpoint(self):
        ops = FakeOps(5)
        report = self.run_chase(ops, fast=True, cadence=2)
        self.assertEqual(ops.full_calls, [0, 2, 4, 5])
        self.assertEqual(ops.build_calls, [1, 3, 5])
        self.assertEqual(len(report["accepted_candidates"]), 5)
        self.assertEqual(report["tentative_additions"], 0)
        self.assertEqual(report["final_blocker"]["module"], "sceUmdUser")
        self.assertEqual(report["final_blocker"]["nid"], 0x8EF08FCE)

    def test_verification_failure_restores_checkpoint(self):
        ops = FakeOps(4, fail_at=4)
        report = self.run_chase(ops, fast=True, cadence=2)
        self.assertTrue(report["error"])
        self.assertEqual(ops.index, 2)
        self.assertTrue(ops.restored)
        self.assertEqual(report["rollback"]["tentative_additions"], 2)
        self.assertEqual(report["last_known_good_seed_count"], 20)
        self.assertEqual(ops.build_calls[-1], 2)

    def test_malformed_tentative_event_rolls_back(self):
        ops = FakeOps(4, bad_at=3)
        report = self.run_chase(ops, fast=True, cadence=2)
        self.assertTrue(report["error"])
        self.assertEqual(ops.index, 2)
        self.assertEqual(report["rollback"]["tentative_additions"], 1)

    def test_ambiguous_semantic_and_budget_stops(self):
        ops = FakeOps(3, ambiguous_at=0)
        report = self.run_chase(ops, fast=True)
        self.assertEqual(ops.index, 0)
        self.assertTrue(report["stop"].startswith("ambiguous_boundary"))
        ops = FakeOps(0)
        report = self.run_chase(ops, fast=True)
        self.assertEqual(ops.index, 0)
        self.assertEqual(report["stop"], "missing_hle")
        ops = FakeOps(3)
        report = self.run_chase(ops, fast=True, budget=1)
        self.assertEqual(ops.index, 1)
        self.assertEqual(ops.full_calls, [0, 1])
        self.assertEqual(report["stop"], "addition_budget")
        self.assertEqual(ops.proof_calls, [0, 1])

    def test_check_only_still_proves_candidate(self):
        ops = FakeOps(1)
        report = self.run_chase(ops, fast=False, check_only=True)
        self.assertEqual(ops.proof_calls, [0])
        self.assertEqual(ops.index, 0)
        self.assertEqual(report["stop"], "check_only")

    def test_semantic_after_tentative_addition_gets_final_checkpoint(self):
        ops = FakeOps(1)
        report = self.run_chase(ops, fast=True, cadence=10)
        self.assertEqual(ops.full_calls, [0, 1])
        self.assertEqual(report["stop"], "missing_hle")
        self.assertEqual(report["final_verification"], "PASS")


if __name__ == "__main__":
    unittest.main()
