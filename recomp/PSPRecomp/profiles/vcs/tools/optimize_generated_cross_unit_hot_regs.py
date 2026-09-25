#!/usr/bin/env python3
"""Share dominant Allegrex GPR/FPR state across generated AOT units.

Transforms an already-generated corpus, so the commercial ELF is not
required for this checkpoint. The matching pass in tools/codegen_main.cpp keeps
future regeneration equivalent. The transform is intentionally idempotent.
"""
from pathlib import Path
import argparse
import re

HOT_GPRS = (2, 4, 5, 6, 7, 29, 31)
HOT_FPRS = (12, 13, 14, 15, 20, 22)
OLD_SIGNED = re.compile(
    r'^(\s*)if \(!ctx\.(execute_signed_(?:add|sub)\([^)]*\))\) '
    r'\{ (rt\.arithmetic_overflow\([^;]*\); return;) \}\s*$'
)
NEW_SIGNED = re.compile(
    r'^(\s*)\{ const bool signed_ok = ctx\.(execute_signed_(?:add|sub)\([^)]*\));\s*$'
)


def _lower_signed_helpers(body: str) -> str:
    """Synchronize rare ctx-internal signed ADD/SUB helpers safely.

    Cache reload happens before the possible overflow return, so the outer wrapper
    can never flush stale hot-register state over an updated AllegrexContext.
    """
    src = body.splitlines(True)
    out: list[str] = []
    i = 0
    while i < len(src):
        line = src[i]
        stripped = line.rstrip('\r\n')
        old = OLD_SIGNED.match(stripped)
        new = NEW_SIGNED.match(stripped)
        if old:
            indent, call, overflow = old.groups()
            if not out or out[-1].strip() != 'hot_regs.flush_to(ctx);':
                out.append(indent + 'hot_regs.flush_to(ctx);\n')
            out.append(indent + '{ const bool signed_ok = ctx.' + call + ';\n')
            out.append(indent + '  hot_regs.reload_from(ctx);\n')
            out.append(indent + '  if (!signed_ok) { ' + overflow + ' } }\n')
            i += 1
            continue
        if new:
            indent = new.group(1)
            if not out or out[-1].strip() != 'hot_regs.flush_to(ctx);':
                out.append(indent + 'hot_regs.flush_to(ctx);\n')
            out.append(line)
            # Older partial transformations may already have one or several reloads.
            while i + 1 < len(src) and src[i + 1].strip() == 'hot_regs.reload_from(ctx);':
                i += 1
            out.append(indent + '  hot_regs.reload_from(ctx);\n')
            i += 1
            continue
        # Collapse duplicate synchronization lines from any interrupted old run.
        if (line.strip() in ('hot_regs.flush_to(ctx);', 'hot_regs.reload_from(ctx);') and
                out and out[-1].strip() == line.strip()):
            i += 1
            continue
        out.append(line)
        i += 1
    return ''.join(out)


def transform(path: Path) -> tuple[int, int]:
    s = path.read_text(errors='ignore')
    orig = s

    # New generated-entry ABI.
    s = re.sub(
        r'(void recomp_unit_\d+_entry\(Runtime &rt, AllegrexContext &ctx, '
        r'std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem)\) \{',
        r'\1, AotHotRegisterCache & PSPRECOMP_RESTRICT hot_regs) {', s, count=1)

    # Carry the same AOT memory view + hot-register cache through generated chains.
    s = s.replace('(ctx, &aot_mem)', '(ctx, &aot_mem, &hot_regs)')
    s = s.replace('rt.invoke_native_fast_path(0x088B1554u, ctx);',
                  'rt.invoke_native_fast_path(0x088B1554u, ctx, &hot_regs);')
    s = s.replace('rt.invoke_native_fast_path(0x088B1780u, ctx);',
                  'rt.invoke_native_fast_path(0x088B1780u, ctx, &hot_regs);')
    s = s.replace('if (++local_transfers < 256u)', 'if (++local_transfers < 2048u)')

    # Only transform the entry body. Helper functions before it and the outer
    # wrapper after it retain the ordinary full-context ABI.
    m = re.search(r'void recomp_unit_\d+_entry\([^\n]+\) \{', s)
    if not m:
        return (0, 0)
    body_start = m.end()
    body_end = s.find('\n}\n\nvoid recomp_unit_', body_start)
    if body_end < 0:
        raise RuntimeError(f'entry end not found: {path}')
    body = s[body_start:body_end]

    replacements = 0
    for reg in HOT_GPRS:
        old, new = f'ctx.gpr[{reg}]', f'hot_regs.g{reg}'
        replacements += body.count(old)
        body = body.replace(old, new)
    for reg in HOT_FPRS:
        old, new = f'ctx.fpr[{reg}]', f'hot_regs.f{reg}'
        replacements += body.count(old)
        body = body.replace(old, new)

    body = _lower_signed_helpers(body)
    s = s[:body_start] + body + s[body_end:]

    # Outer dispatch owns cache lifetime; nested generated calls share it.
    unit = re.search(
        r'void (recomp_unit_\d+)\(Runtime &rt, AllegrexContext &ctx\) \{\n'
        r'\s*auto aot_mem = rt\.memory\(\)\.aot_fast_view\(\);\n'
        r'\s*\1_entry\(rt, ctx, 0u, aot_mem\);\n\}', s)
    if unit:
        name = unit.group(1)
        repl = (f'void {name}(Runtime &rt, AllegrexContext &ctx) {{\n'
                f'    auto aot_mem = rt.memory().aot_fast_view();\n'
                f'    AotHotRegisterCache hot_regs(ctx);\n'
                f'    {name}_entry(rt, ctx, 0u, aot_mem, hot_regs);\n'
                f'    hot_regs.flush_to(ctx);\n}}')
        s = s[:unit.start()] + repl + s[unit.end():]
    elif 'AotHotRegisterCache hot_regs(ctx);' not in s:
        raise RuntimeError(f'wrapper not found: {path}')

    if s != orig:
        path.write_text(s)
        return (1, replacements)
    return (0, replacements)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('generated', type=Path)
    args = ap.parse_args()
    changed = refs = 0
    for path in sorted(args.generated.glob('generated_unit_*.cpp')):
        c, r = transform(path)
        changed += c
        refs += r

    header = args.generated / 'generated_units.hpp'
    hs = header.read_text()
    hs2 = hs.replace('std::uint16_t, GuestMemory::AotFastView &);',
                     'std::uint16_t, GuestMemory::AotFastView &, AotHotRegisterCache &);')
    if 'struct AotHotRegisterCache;' not in hs2:
        hs2 = hs2.replace('struct AllegrexContext;\n',
                          'struct AllegrexContext;\nstruct AotHotRegisterCache;\n')
    if hs2 != hs:
        header.write_text(hs2)
    print(f'changed_units={changed} hot_regs_references_rewritten={refs}')


if __name__ == '__main__':
    main()
