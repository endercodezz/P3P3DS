"""Contact sheet of PC-runner frame dumps, each labelled with its vblank.

usage: contact_sheet.py <frames dir> <events json> <out.png> [first_vblank] [last_vblank] [--cols N] [--max N]

Needs a run with --frames-dir and --dump-events (frame events carry the
virtual time; vblank = time_us / 16683). With more frames than --max
(default 24) in the range, frames are picked evenly. Requires Pillow.
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    args = sys.argv[1:]
    opts = {'--cols': 6, '--max': 24}
    for key in list(opts):
        if key in args:
            i = args.index(key)
            opts[key] = int(args[i + 1])
            del args[i:i + 2]
    frames_dir, events_path, out = Path(args[0]), args[1], args[2]
    lo = int(args[3]) if len(args) > 3 else 0
    hi = int(args[4]) if len(args) > 4 else 10**9

    ev = json.load(open(events_path))
    evs = ev['events'] if isinstance(ev, dict) else ev
    vblank_of = {e['fields']['index']: int(e['fields']['time_us'] / 16683) for e in evs if e.get('type') == 'frame'}
    items = []
    for f in sorted(frames_dir.glob('frame_*.bmp')):
        vb = vblank_of.get(int(f.stem.split('_')[1]))
        if vb is not None and lo <= vb <= hi:
            items.append((vb, f))
    items.sort()
    if len(items) > opts['--max']:
        step = len(items) / opts['--max']
        items = [items[int(i * step)] for i in range(opts['--max'])]

    cols, tw, th = opts['--cols'], 240, 136
    rows = max(1, (len(items) + cols - 1) // cols)
    sheet = Image.new('RGB', (cols * tw, rows * (th + 14)), (30, 30, 30))
    draw = ImageDraw.Draw(sheet)
    for i, (vb, f) in enumerate(items):
        x, y = (i % cols) * tw, (i // cols) * (th + 14)
        sheet.paste(Image.open(f).convert('RGB').resize((tw, th)), (x, y + 14))
        draw.text((x + 4, y + 1), f'vbl {vb}', fill=(255, 255, 0))
    sheet.save(out)
    print(out, len(items), 'frames:', ' '.join(str(vb) for vb, _ in items))


main()
