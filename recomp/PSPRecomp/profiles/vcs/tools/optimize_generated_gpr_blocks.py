#!/usr/bin/env python3
"""Conservative basic-block GPR register cache for generated AOT units.

This pass only rewrites guest labels that contain no Runtime calls and no
AllegrexContext execute_* helper. Registers referenced at least three times in
one block are copied to host locals, writes stay local, and dirty values are
committed before every control-flow exit. Each guest label is wrapped in its
own C++ scope so arbitrary generated gotos never cross local initialization.

It is intentionally conservative: blocks containing rt.* are left untouched
because a chained/HLE/runtime call can observe or mutate the complete Allegrex
context. AotFastView/FPR/VFPU operations remain eligible because they do not
mutate GPR state behind the generated code's back.
"""
from __future__ import annotations

import argparse
import collections
import pathlib
import re
from dataclasses import dataclass

LABEL_RE = re.compile(r"(?m)^L_[0-9A-F]+:\n")
GPR_RE = re.compile(r"ctx\.gpr\[([1-9]|[12][0-9]|3[01])\]")


@dataclass
class Stats:
    blocks_cached: int = 0
    registers_cached: int = 0
    occurrences_replaced: int = 0
    dirty_registers: int = 0

    def add(self, other: "Stats") -> None:
        self.blocks_cached += other.blocks_cached
        self.registers_cached += other.registers_cached
        self.occurrences_replaced += other.occurrences_replaced
        self.dirty_registers += other.dirty_registers


def _is_assignment(line: str, name: str) -> bool:
    # Generated writes are simple `name = (...)`; exclude ==, <=, >=, !=.
    return re.search(rf"(?<![=!<>])\b{re.escape(name)}\s*=(?!=)", line) is not None


def _transform_block(label: str, block: str, threshold: int) -> tuple[str, Stats]:
    stats = Stats()
    # Runtime helpers may inspect/mutate ctx.gpr and therefore are explicit
    # synchronization boundaries. execute_signed_add/sub also writes a GPR
    # internally, so keep those blocks in the reference representation.
    if "rt." in block or "ctx.execute_" in block:
        return label + block, stats

    counts = collections.Counter(GPR_RE.findall(block))
    selected = sorted(int(reg) for reg, count in counts.items() if count >= threshold)
    if not selected:
        return label + block, stats

    dirty = {
        reg for reg in selected
        if re.search(rf"ctx\.gpr\[{reg}\]\s*=(?!=)", block) is not None
    }

    # Token-exact replacement. Replacing gpr[1] cannot touch gpr[10].
    for reg in selected:
        block = block.replace(f"ctx.gpr[{reg}]", f"g{reg}")

    lines = block.splitlines(keepends=True)
    rewritten: list[str] = []
    rewritten.extend(f"    std::uint32_t g{reg} = ctx.gpr[{reg}];\n" for reg in selected)

    active_dirty: set[int] = set()

    def emit_flush() -> None:
        nonlocal active_dirty
        if not active_dirty:
            return
        for reg in sorted(active_dirty):
            rewritten.append(f"    ctx.gpr[{reg}] = g{reg};\n")
        active_dirty.clear()

    for line in lines:
        # The generated delay-slot branch shape is:
        #   const bool branch_taken = ...;
        #   <delay slot writes>
        #   if (branch_taken) { goto ...; }
        # Flush before the branch decision so BOTH taken and fallthrough paths
        # see the committed delayed writes. Direct conditional-goto lines are
        # handled by the generic `goto` condition below.
        if "if (branch_taken)" in line and active_dirty:
            emit_flush()

        # A dirty assignment on the same line as an exit does not occur in the
        # automatic corpus. Mark writes before normal control-flow detection so
        # future simple `gN = expr; return;` forms still commit conservatively.
        for reg in dirty:
            if _is_assignment(line, f"g{reg}"):
                active_dirty.add(reg)

        if ("goto " in line or "return;" in line) and active_dirty:
            # If this is the nested goto inside `if (branch_taken)`, the flush
            # above already synchronized and active_dirty is empty.
            emit_flush()

        rewritten.append(line)

    emit_flush()

    stats.blocks_cached = 1
    stats.registers_cached = len(selected)
    stats.occurrences_replaced = sum(counts[str(reg)] for reg in selected)
    stats.dirty_registers = len(dirty)
    return label + "{\n" + "".join(rewritten) + "}\n", stats


def transform_text(text: str, threshold: int = 3) -> tuple[str, Stats]:
    matches = list(LABEL_RE.finditer(text))
    if not matches:
        return text, Stats()

    output: list[str] = []
    last = 0
    total = Stats()
    for index, match in enumerate(matches):
        block_start = match.end()
        if index + 1 < len(matches):
            block_end = matches[index + 1].start()
        else:
            # Stop at the end of the generated _entry function, before its
            # outer wrapper. This prevents swallowing registration code into
            # the final guest-label scope.
            block_end = text.find("\n}\n\nvoid ", block_start)
            if block_end < 0:
                block_end = len(text)

        output.append(text[last:match.start()])
        transformed, stats = _transform_block(
            text[match.start():block_start], text[block_start:block_end], threshold)
        output.append(transformed)
        total.add(stats)
        last = block_end

    output.append(text[last:])
    return "".join(output), total


def optimize_file(path: pathlib.Path, threshold: int, check: bool) -> Stats:
    original = path.read_text(encoding="utf-8")
    transformed, stats = transform_text(original, threshold)
    if not check and transformed != original:
        path.write_text(transformed, encoding="utf-8", newline="\n")
    return stats


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", type=pathlib.Path)
    parser.add_argument("--threshold", type=int, default=3)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    total = Stats()
    files: list[pathlib.Path] = []
    for path in args.paths:
        if path.is_dir():
            files.extend(sorted(path.glob("generated_unit_*.cpp")))
        else:
            files.append(path)
    for path in files:
        total.add(optimize_file(path, args.threshold, args.check))

    print(
        f"GPR block cache: files={len(files)} blocks={total.blocks_cached} "
        f"locals={total.registers_cached} occurrences={total.occurrences_replaced} "
        f"dirty={total.dirty_registers} threshold={args.threshold} check={int(args.check)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
