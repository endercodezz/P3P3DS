#!/usr/bin/env python3
"""Read a 480x272 RGBA TGA, check center pixel against (r,g,b) with tolerance.
Usage: check_center_pixel.py <tga> <exp_r> <exp_g> <exp_b> <tol>"""
import sys
data = open(sys.argv[1], "rb").read()
cx = (480 * 272 // 2) * 4 + 18  # 18-byte TGA header, RGBA stride 4
b, g, r, a = data[cx], data[cx + 1], data[cx + 2], data[cx + 3]  # TGA stores BGRA
exp_r, exp_g, exp_b, tol = (int(x) for x in sys.argv[2:6])
if abs(r - exp_r) <= tol and abs(g - exp_g) <= tol and abs(b - exp_b) <= tol:
    print(f"PASS {r} {g} {b} {a}")
    sys.exit(0)
print(f"FAIL got=({r},{g},{b},{a}) want=({exp_r},{exp_g},{exp_b}) tol={tol}",
      file=sys.stderr)
sys.exit(1)
