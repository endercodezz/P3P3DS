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
                "blocker": {"type": "missing_guest_function", "pc": 4096, "caller": 512, "word": 0x0C000400, "target": 4096, "ra": 520, "thread": 2},
                "graphics": {"writer": False, "vram_operations": 0},
                "events": [{"type": "guest_transfer", "pc": 512, "thread": 2, "fields": {"word": 0x0C000400, "target": 4096}}]}

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
        self.assertEqual(chase.candidate(e), "graphics writer")

    def test_manifest_duplicate_and_containment(self):
        with tempfile.TemporaryDirectory(dir=ROOT / ".tmp") as d:
            a, b, out = [Path(d) / name for name in ("a.csv", "b.csv", "out.csv")]
            a.write_bytes(chase.encoded([{"name": "a", "address": "0x1000", "size": "cfg"}]))
            b.write_bytes(chase.encoded([]));chase.merge(a, b, out)
            self.assertEqual(a.read_bytes(), out.read_bytes())
            b.write_bytes(a.read_bytes())
            with self.assertRaises(ValueError):chase.merge(a, b, out)
        with self.assertRaises(ValueError):chase.contained(ROOT.parent / "escape")


if __name__ == "__main__":
    unittest.main()
