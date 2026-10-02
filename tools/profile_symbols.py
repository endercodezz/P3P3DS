"""Map a PC-runner sample file (--sample) to functions.

usage: profile_symbols.py <runner.exe> <samples.txt> [top N] [--nm <nm executable>]

Reads the executable's symbols with nm, attributes every sampled address to
the nearest preceding text symbol and prints the hottest functions, then the
same grouped by AOT unit / HLE area.
"""
import bisect
import collections
import re
import subprocess
import sys


def main():
    args = sys.argv[1:]
    nm = 'nm'
    if '--nm' in args:
        i = args.index('--nm')
        nm = args[i + 1]
        del args[i:i + 2]
    exe, samples_path = args[0], args[1]
    top = int(args[2]) if len(args) > 2 else 40
    out = subprocess.run([nm, '-C', '--defined-only', exe], capture_output=True, text=True, errors='replace').stdout
    syms = []
    for line in out.splitlines():
        parts = line.split(' ', 2)
        if len(parts) == 3 and parts[1] in 'tTwW' and not parts[2].startswith('.'):
            syms.append((int(parts[0], 16), parts[2]))
    syms.sort()
    addrs = [a for a, _ in syms]
    per_fn = collections.Counter()
    total = 0
    link_base = 0x140000000  # PE image base of the MinGW runner
    shift = 0
    modules = []
    for line in open(samples_path):
        parts = line.split()
        if parts[0] == 'module':
            base, size, name = int(parts[1], 16), int(parts[2], 16), ' '.join(parts[3:])
            if not modules:
                shift = link_base - base  # the first module is the runner itself
            modules.append((base, size, name))
            continue
        a, n = int(parts[0], 16), int(parts[1])
        total += n
        dll = next((m for m in modules[1:] if m[0] <= a < m[0] + m[1]), None)
        if dll:
            per_fn['[' + dll[2] + ']'] += n
            continue
        a += shift
        i = bisect.bisect_right(addrs, a) - 1
        per_fn[syms[i][1] if i >= 0 else '?'] += n
    print(f'samples: {total}')
    for name, n in per_fn.most_common(top):
        print(f'{100.0 * n / total:6.2f}%  {n:8d}  {name[:150]}')
    groups = collections.Counter()
    for name, n in per_fn.items():
        m = re.match(r'psprecomp::recomp_unit_(\d+)', name)
        if m:
            key = 'AOT units'
        elif name.startswith('['):
            key = 'system DLLs (CRT heap, kernel)'
        elif 'p3p3ds::hle::GeManager' in name or 'p3p3ds::ge::' in name or 'Renderer' in name:
            key = 'GE / rendering'
        elif 'p3p3ds::' in name:
            key = 'other p3p3ds (HLE, VFS, ...)'
        elif 'psprecomp::' in name:
            key = 'psprecomp runtime'
        elif any(k in name for k in ('malloc', 'free', 'operator new', 'operator delete', 'mem', 'str')):
            key = 'libc (alloc, memcpy, strings)'
        else:
            key = 'other'
        groups[key] += n
    print()
    for key, n in groups.most_common():
        print(f'{100.0 * n / total:6.2f}%  {key}')


main()
