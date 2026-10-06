#!/usr/bin/env python3
"""
Gera o PNG fonte 320x480 do fundo gradiente ditherizado
(assets/backgrounds/bg_dither.png) de forma procedural (Pillow), no mesmo
espirito de tools/generate_icon_art.py/generate_progress_art.py: um script
project-owned, deterministico e reproduzivel a partir do codigo fonte.

Algoritmo IDENTICO ao do sketch aprovado (001-C, usado como fundo do
003-C): .planning/sketches/themes/gen_dither_bg.py -> bg-dither-scan.png.
Plano 02.1-14 reescreveu este gerador porque a versao anterior divergia
do sketch em tres pontos que a usuaria viu na tela ("pixels maiores/mais
pesados"):
  1. escolhia so' entre os DOIS stops vizinhos por pixel -- o sketch
     interpola o gradiente e quantiza cada canal em 10 niveis com
     limiar Bayer 4x4 (muito mais degraus, granulacao mais fina);
  2. aplicava as scanlines na resolucao 80x120 e o LVGL esticava 4x em
     runtime -> cada scanline virava uma faixa de 4px;
  3. stops em 0/42/68/100% em vez dos tres segmentos iguais do sketch.

Pipeline do sketch, reproduzido aqui passo a passo:
  - grade baixa 80x120: cor do gradiente de 4 stops (3 segmentos iguais),
    + limiar Bayer 4x4 * (255/10), quantizada em 10 niveis por canal;
  - sobe NEAREST pra 320x480 (cada pixel baixo vira um bloco de 4x4);
  - so' DEPOIS escurece 1 linha a cada 3 (x0.85) -> scanline de 1px real.

O PNG sai 320x480 e e' desenhado 1:1 (sem stretch) por theme.c. Rodar
duas vezes produz um PNG pixel-identico; `--check` compara o resultado
com o PNG do sketch.

Uso: python3 tools/generate_bg_dither.py [--check]
"""
import sys
from pathlib import Path

from PIL import Image, ImageChops

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_PATH = REPO_ROOT / "assets" / "backgrounds" / "bg_dither.png"
SKETCH_REF = REPO_ROOT / ".planning" / "sketches" / "themes" / "bg-dither-scan.png"

W, H = 320, 480
LOWW, LOWH = 80, 120
PALETTE_STEPS = 10

# Matriz de dithering ordenado Bayer 4x4 classica (valores 0..15).
BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]

# 4 stops verticais (topo -> base), hex exatos de fundo-e-ambiente.md.
STOPS = [
    (0x1C, 0x0A, 0x3D),
    (0x7A, 0x15, 0x60),
    (0xC8, 0x1F, 0x4C),
    (0xE8, 0x63, 0x0F),
]

SCANLINE_EVERY = 3
SCANLINE_FACTOR = 0.85


def color_at(t):
    seg = min(2, int(t * 3))
    local_t = (t * 3) - seg
    a, b = STOPS[seg], STOPS[seg + 1]
    return tuple(a[i] + (b[i] - a[i]) * local_t for i in range(3))


def quantize(v, levels):
    step = 255 / (levels - 1)
    return round(v / step) * step


def generate() -> Image.Image:
    low = Image.new("RGB", (LOWW, LOWH))
    px = low.load()
    amt = 255 / PALETTE_STEPS
    for y in range(LOWH):
        t = y / (LOWH - 1)
        r, g, b = color_at(t)
        for x in range(LOWW):
            threshold = (BAYER4[y % 4][x % 4] / 16) - 0.5
            rr = min(255, max(0, quantize(r + threshold * amt, PALETTE_STEPS)))
            gg = min(255, max(0, quantize(g + threshold * amt, PALETTE_STEPS)))
            bb = min(255, max(0, quantize(b + threshold * amt, PALETTE_STEPS)))
            px[x, y] = (int(rr), int(gg), int(bb))

    big = low.resize((W, H), Image.NEAREST).convert("RGB")
    bpx = big.load()
    for y in range(0, H, SCANLINE_EVERY):
        for x in range(W):
            r, g, b = bpx[x, y]
            bpx[x, y] = (int(r * SCANLINE_FACTOR), int(g * SCANLINE_FACTOR), int(b * SCANLINE_FACTOR))
    return big


def main() -> None:
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    im = generate()
    im.save(OUT_PATH)
    print(f"wrote {OUT_PATH} ({W}x{H})")

    if "--check" in sys.argv[1:]:
        ref = Image.open(SKETCH_REF).convert("RGB")
        if ref.size != im.size:
            sys.exit(f"MISMATCH: size {im.size} != sketch {ref.size}")
        if ImageChops.difference(im, ref).getbbox() is not None:
            sys.exit("MISMATCH: pixels differ from the sketch's bg-dither-scan.png")
        print(f"OK: pixel-identical to {SKETCH_REF.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
