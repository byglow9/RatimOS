"""Throwaway generator for the sketch-only dithered gradient background asset.
Not part of the firmware build — lives under .planning/sketches/ only.
Produces themes/bg-dither.png (320x480) and themes/bg-dither-scan.png (with scanlines).
"""
from PIL import Image
import math

W, H = 320, 480
LOWW, LOWH = 80, 120
PALETTE_STEPS = 10

BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]

stops = [
    (0x1c, 0x0a, 0x3d),
    (0x7a, 0x15, 0x60),
    (0xc8, 0x1f, 0x4c),
    (0xe8, 0x63, 0x0f),
]

def color_at(t):
    seg = min(2, int(t * 3))
    local_t = (t * 3) - seg
    a, b = stops[seg], stops[seg + 1]
    return tuple(a[i] + (b[i] - a[i]) * local_t for i in range(3))

def quantize(v, levels):
    step = 255 / (levels - 1)
    return round(v / step) * step

def build(scanlines):
    low = Image.new("RGB", (LOWW, LOWH))
    px = low.load()
    for y in range(LOWH):
        t = y / (LOWH - 1)
        r, g, b = color_at(t)
        for x in range(LOWW):
            threshold = (BAYER4[y % 4][x % 4] / 16) - 0.5
            amt = 255 / PALETTE_STEPS
            rr = min(255, max(0, quantize(r + threshold * amt, PALETTE_STEPS)))
            gg = min(255, max(0, quantize(g + threshold * amt, PALETTE_STEPS)))
            bb = min(255, max(0, quantize(b + threshold * amt, PALETTE_STEPS)))
            px[x, y] = (int(rr), int(gg), int(bb))
    big = low.resize((W, H), Image.NEAREST)
    if scanlines:
        big = big.convert("RGB")
        bpx = big.load()
        for y in range(0, H, 3):
            for x in range(W):
                r, g, b = bpx[x, y]
                bpx[x, y] = (int(r * 0.85), int(g * 0.85), int(b * 0.85))
    return big

build(False).save("bg-dither.png")
build(True).save("bg-dither-scan.png")
print("done")
