#!/usr/bin/env python3
"""
Gera a arte dos TEMAS do RatimOS (plano 02.1-16, sketch 010):
src/ratimos/theme_art.c e theme_art.h. Porte DIRETO das funcoes do canvas de
.planning/sketches/010-temas/index.html (`rnd`, `bgStars`, `sunsetScene`,
`chessRender`, `bgChessHome`, `thumb()`), em Pillow, sem pacote novo.

Fundos 320x480 sao SEMPRE RGB565 cru: uma imagem indexada de tela cheia passaria
pelo decoder do LVGL e alocaria buffer no heap (motivo registrado no 02.1-14).

Gera:
  Fundos estaticos da HOME / sistema (sem animacao; o escurecimento da home do
  castelo, 20%, NAO entra na imagem -- e' aplicado em tempo de execucao):
    ratimos_home_bg_noite_desc    bgStars(c, false): fases fixas 1 + i%3
    ratimos_home_bg_castelo_desc  sunsetScene(c, false, 'home'): horizonte 450, janelas acesas
    ratimos_home_bg_xadrez_desc   chessRender(0, 70, 0.85), sem linha de horizonte
  Fundo do BOOT do castelo:
    ratimos_boot_bg_castelo_desc  sunsetScene(c, false, 'boot'): horizonte 466, ox 150,
                                  janelas pintadas ACESAS; as que apagam sao repintadas em
                                  runtime com a tabela ratimos_castelo_windows
  Fundo do BOOT da Noite (nenhuma imagem 320x480):
    3 faixas, tabela de 70 estrelas {x, y, period_ms, big, col}, sprite da lua 27x27 com alfa
  Miniaturas 40x60 RGB565 (LANCZOS, como o thumb() do 010), 8 no total:
    ratimos_thumb_home_<tema>_desc    fundo da home de cada tema (a do castelo ja escurecida 20%)
    ratimos_thumb_bootbg_<tema>_desc  fundo de boot de cada tema, num quadro estatico com o
                                      escurecimento de boot

Deterministico: rodar duas vezes gera arquivos identicos. `rnd` e' identica em
double no Python e no JS (conferido contra o node: 70 estrelas, 0 diferencas).

Uso: python3 tools/generate_theme_art.py
"""
import math
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
BG_DITHER = REPO_ROOT / "assets" / "backgrounds" / "bg_dither.png"
OUT_C = REPO_ROOT / "src" / "ratimos" / "theme_art.c"
OUT_H = REPO_ROOT / "src" / "ratimos" / "theme_art.h"

W, H = 320, 480


def hexrgb(h):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def jsround(v):
    return int(math.floor(v + 0.5))


def rnd(i):
    x = math.sin(i * 127.1 + 311.7) * 43758.5453
    return x - math.floor(x)


class Canvas:
    def __init__(self, w=W, h=H):
        self.im = Image.new("RGB", (w, h), (0, 0, 0))
        self.w, self.h = w, h

    def px(self, x, y, w, h, col):
        x0, y0, x1, y1 = max(0, x), max(0, y), min(self.w, x + w), min(self.h, y + h)
        if x1 > x0 and y1 > y0:
            self.im.paste(col, (x0, y0, x1, y1))


# ---------------------------------------------------------------- Noite estrelada
NOITE_BANDS = ["#07030f", "#0b0518", "#110822"]
STAR_COUNT = 70


def star_table():
    stars = []
    for i in range(STAR_COUNT):
        x = int(math.floor(rnd(i) * 320))
        y = int(math.floor(rnd(i + 99) * 480))
        period = 500 + rnd(i + 7) * 900
        big = 1 if rnd(i + 3) > 0.9 else 0
        col = "#a997ba" if rnd(i + 5) > 0.5 else "#6a4f8e"
        stars.append((x, y, jsround(period), big, col))
    return stars


MOON = dict(mx=262, my=56, r=13)


def draw_moon(c, ox=0, oy=0):
    mx, my, r = MOON["mx"] - ox, MOON["my"] - oy, MOON["r"]
    for y in range(-r, r + 1):
        w = int(math.floor(math.sqrt(r * r - y * y)))
        c.px(mx - w, my + y, 2 * w + 1, 1, hexrgb("#d9cfe6"))
    for y in range(-r, r + 1):
        w = int(math.floor(math.sqrt(r * r - y * y)))
        c.px(mx + w - 3, my + y, 3, 1, hexrgb("#a997ba"))
    c.px(mx - 5, my - 4, 3, 3, hexrgb("#b8abc9"))
    c.px(mx + 2, my + 4, 4, 3, hexrgb("#b8abc9"))
    c.px(mx - 3, my + 6, 2, 2, hexrgb("#b8abc9"))


def bg_stars_static():
    """bgStars(c, false): fases fixas 1 + i%3."""
    c = Canvas()
    c.px(0, 0, 320, 480, hexrgb(NOITE_BANDS[0]))
    c.px(0, 160, 320, 160, hexrgb(NOITE_BANDS[1]))
    c.px(0, 320, 320, 160, hexrgb(NOITE_BANDS[2]))
    stars = star_table()
    for i, (x, y, _period, big, col) in enumerate(stars):
        ph = 1 + (i % 3)
        c.px(x, y, 1, 1, hexrgb(col))
        if big and ph >= 2:
            for a, b in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                c.px(x + a, y + b, 1, 1, hexrgb("#6a4f8e"))
    draw_moon(c)
    return c.im


def moon_sprite():
    """Lua 27x27 com alfa (origem em mx-13, my-13): o disco inteiro e' opaco
    (crescente e crateras ficam dentro dele), fora e' transparente."""
    r = MOON["r"]
    sz = 2 * r + 1
    ox, oy = MOON["mx"] - r, MOON["my"] - r
    rgb = Canvas(sz, sz)
    draw_moon(rgb, ox, oy)
    disc = Canvas(sz, sz)
    for y in range(-r, r + 1):
        w = int(math.floor(math.sqrt(r * r - y * y)))
        disc.px(r - w, r + y, 2 * w + 1, 1, (255, 255, 255))
    out = rgb.im.convert("RGBA")
    out.putalpha(disc.im.convert("L"))
    return out, ox, oy


# ---------------------------------------------------------------- Castelo ao entardecer
CASTELO_BANDS = ["#0c0420", "#12062a", "#1c0a3d", "#26093f", "#36104c", "#4a0f55", "#7a1560", "#a8193f", "#d8381f", "#ee6a12"]
WIN_ON, WIN_OFF, WIN_TOP = "#ffc04a", "#7a4a10", "#1f0a1c"


def sunset_scene(layout):
    """sunsetScene(c, false, layout): janelas acesas. Devolve (imagem, janelas)."""
    c = Canvas()
    Hh = 450 if layout == "home" else 466
    ox = 150
    bands = CASTELO_BANDS
    bh = int(math.ceil(Hh / len(bands)))
    for i, col in enumerate(bands):
        c.px(0, i * bh, 320, bh, hexrgb(col))
    for i in range(1, len(bands)):
        for x in range(0, 320, 2):
            c.px(x, i * bh - 1, 1, 1, hexrgb(bands[i]))
            c.px(x + 1, i * bh, 1, 1, hexrgb(bands[i - 1]))

    sx, sy, R = 74, Hh - 30, 30

    def disk(r, col, dither):
        for y in range(-r, r + 1):
            w = int(math.floor(math.sqrt(r * r - y * y)))
            for x in range(-w, w + 1):
                if not dither or ((x + y) & 1) == 0:
                    c.px(sx + x, sy + y, 1, 1, hexrgb(col))

    disk(R + 10, "#f08a30", True)
    disk(R + 5, "#f6a040", True)
    for y in range(-R, R + 1):
        w = int(math.floor(math.sqrt(R * R - y * y)))
        if y > 4 and (y % 6 == 0 or y % 6 == 1) and y < R:
            continue
        col = "#ffe08a" if y < -R / 2 else ("#ffc861" if y < 0 else "#ffa94a")
        c.px(sx - w, sy + y, 2 * w + 1, 1, hexrgb(col))

    peaks = [(-10, 46), (40, 62), (88, 40), (140, 70), (196, 44), (250, 58), (300, 50), (340, 40)]
    for x in range(320):
        top = Hh
        for p0, ph in peaks:
            d = abs(x - p0)
            t = Hh - ph + int(math.floor(d * 1.15))
            if t < top:
                top = t
        top = top - (top % 2)
        nearest = peaks[0]
        for p in peaks:
            if abs(x - p[0]) < abs(x - nearest[0]):
                nearest = p
        lit = x < nearest[0]
        c.px(x, top, 1, 480 - top, hexrgb("#5a1650" if lit else "#3a0c40"))
        if lit:
            c.px(x, top, 1, 2, hexrgb("#9a2a50"))

    S, S2, ROOF, ROOF_LIT = "#140513", "#1f0a1c", "#8a0a18", "#d01e24"
    K, AX, gy = 1.2, ox + 84, Hh - 8

    def X(n):
        return AX + jsround((n - 94) * K)

    def Y(d):
        return gy - jsround(d * K)

    def L(n):
        return jsround(n * K)

    def crenel(x, y, w):
        k = 0
        while k < w:
            c.px(x + k, y - L(4), L(4), L(4), hexrgb(S))
            k += L(6)

    def roof(cx, y, w, h):
        for r in range(h):
            half = int(math.ceil((r + 1) * (w / 2) / h))
            c.px(cx - half, y + r, half, 1, hexrgb(ROOF))
            c.px(cx, y + r, half, 1, hexrgb(ROOF_LIT))
        c.px(cx, y - L(8), 1, L(8), hexrgb("#d9d0e6"))
        c.px(cx + 1, y - L(8), L(5), L(3), hexrgb("#e6010f"))

    windows = []

    def win(x, y, i):
        windows.append((x, y, i))
        c.px(x, y, L(3), L(5), hexrgb(WIN_ON))
        c.px(x, y, L(3), 1, hexrgb(S2))

    c.px(X(44), Y(26), L(100), 480 - Y(26), hexrgb(S))
    crenel(X(44), Y(26), L(100))
    c.px(X(80), Y(62), L(28), 480 - Y(62), hexrgb(S2))
    crenel(X(80), Y(62), L(28))
    roof(X(94), Y(86), L(30), L(20))
    win(X(85), Y(52), 0)
    win(X(99), Y(52), 1)
    win(X(92), Y(40), 2)
    for tx, th, wi in ((36, 46, 3), (138, 40, 6)):
        c.px(X(tx), Y(th), L(14), 480 - Y(th), hexrgb(S2))
        roof(X(tx) + L(7), Y(th + 16), L(18), L(14))
        win(X(tx) + L(5), Y(th - 10), wi)

    near = [(-20, 30), (36, 44), (96, 26), (ox + 20, 40), (ox + 70, 30), (ox + 128, 42), (350, 34)]
    for x in range(320):
        top = 480
        for p0, ph in near:
            t = Hh + 14 - ph + int(math.floor(abs(x - p0) * 0.9))
            if t < top:
                top = t
        top = top - (top % 2)
        nearest = near[0]
        for p in near:
            if abs(x - p[0]) < abs(x - nearest[0]):
                nearest = p
        c.px(x, top, 1, 480 - top, hexrgb("#22061f"))
        if x < nearest[0]:
            c.px(x, top, 1, 1, hexrgb("#6a1c44"))

    ww, wh = L(3), L(5)
    return c.im, windows, (ww, wh)


# ---------------------------------------------------------------- Xadrez
CH_A, CH_B, CH_M = (44, 14, 70), (12, 4, 22), (28, 9, 46)
SKY0, SKY1 = (6, 2, 12), (40, 14, 64)


def chess_render(scroll, hy, dim):
    im = Image.new("RGB", (W, H))
    px = im.load()
    for y in range(H):
        for x in range(W):
            if y < hy:
                t = max(0.0, (y - (hy - 60)) / 60)
                col = [SKY0[k] + (SKY1[k] - SKY0[k]) * t * t for k in range(3)]
            else:
                dy = y - hy + 0.5
                z = 65000 / dy
                u = (x - 160) * z / 160
                v = z + scroll
                chk = CH_A if ((int(math.floor(u / 24)) + int(math.floor(v / 24))) & 1) else CH_B
                fog = min(1.0, max(0.0, (dy - 4) / 70))
                col = [CH_M[k] + (chk[k] - CH_M[k]) * fog for k in range(3)]
            # Uint8ClampedArray: arredonda (metade para par) e limita a 0..255
            px[x, y] = tuple(max(0, min(255, round(col[k] * dim))) for k in range(3))
    return im


def chess_boot_frame():
    """bgChess(c, false) do boot: horizonte 140, sem rolagem, com a linha #4e2277."""
    im = chess_render(0, 140, 1)
    c = Canvas()
    c.im = im
    c.px(0, 140, 320, 1, hexrgb("#4e2277"))
    return c.im


# ---------------------------------------------------------------- utilitarios
def dim_image(im, pct):
    if not pct:
        return im
    f = 1 - pct / 100
    return im.point(lambda v: jsround(v * f))


def thumb(im):
    return im.resize((40, 60), Image.LANCZOS)


def words565(im):
    px = im.load()
    w, h = im.size
    out = []
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            out.append(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    return out


def emit_words(name, words):
    lines = [f"static const uint16_t {name}_map[] = {{"]
    for i in range(0, len(words), 12):
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in words[i:i + 12]) + ",")
    lines.append("};")
    return "\n".join(lines)


def emit_rgb565(name, im):
    w, h = im.size
    base = name[: -len("_desc")]
    return (
        emit_words(base, words565(im))
        + f"\n\nconst lv_image_dsc_t {name} = {{\n"
        f"    .header.magic = LV_IMAGE_HEADER_MAGIC,\n"
        f"    .header.cf = LV_COLOR_FORMAT_RGB565,\n"
        f"    .header.w = {w},\n"
        f"    .header.h = {h},\n"
        f"    .header.stride = {w * 2},\n"
        f"    .data_size = sizeof({base}_map),\n"
        f"    .data = (const uint8_t *) {base}_map,\n"
        f"}};\n"
    ), w * h * 2


def emit_rgb565a8(name, im):
    w, h = im.size
    px = im.load()
    rgb, a8 = [], []
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            rgb.append(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
            a8.append(a)
    base = name[: -len("_desc")]
    arr = [f"static const uint8_t {base}_map[] = {{"]
    data = b"".join(v.to_bytes(2, "little") for v in rgb) + bytes(a8)
    for i in range(0, len(data), 16):
        arr.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",")
    arr.append("};")
    return (
        "\n".join(arr)
        + f"\n\nconst lv_image_dsc_t {name} = {{\n"
        f"    .header.magic = LV_IMAGE_HEADER_MAGIC,\n"
        f"    .header.cf = LV_COLOR_FORMAT_RGB565A8,\n"
        f"    .header.w = {w},\n"
        f"    .header.h = {h},\n"
        f"    .header.stride = {w * 2},\n"
        f"    .data_size = sizeof({base}_map),\n"
        f"    .data = {base}_map,\n"
        f"}};\n"
    ), len(data)


def main():
    dither = Image.open(BG_DITHER).convert("RGB")
    assert dither.size == (W, H)

    noite_home = bg_stars_static()
    castelo_home, _wins_home, _ = sunset_scene("home")
    castelo_boot, windows, (win_w, win_h) = sunset_scene("boot")
    xadrez_home = chess_render(0, 70, 0.85)
    chess_boot = chess_boot_frame()
    noite_boot_static = bg_stars_static()

    # as janelas apagam em runtime: o que esta DEBAIXO delas na imagem tem que ser a cor
    # acesa (nenhuma serra da frente cobrindo a janela)
    cp = castelo_boot.load()
    for (x, y, _i) in windows:
        for yy in range(y + 1, y + win_h):
            for xx in range(x, x + win_w):
                assert cp[xx, yy] == hexrgb(WIN_ON), ("janela coberta", x, y)

    images = [
        ("ratimos_home_bg_noite_desc", noite_home),
        ("ratimos_home_bg_castelo_desc", castelo_home),
        ("ratimos_home_bg_xadrez_desc", xadrez_home),
        ("ratimos_boot_bg_castelo_desc", castelo_boot),
        # miniaturas: home (a do castelo ja escurecida 20% antes de reduzir, como o thumb() do 010)
        ("ratimos_thumb_home_classico_desc", thumb(dither)),
        ("ratimos_thumb_home_noite_desc", thumb(noite_home)),
        ("ratimos_thumb_home_castelo_desc", thumb(dim_image(castelo_home, 20))),
        ("ratimos_thumb_home_xadrez_desc", thumb(xadrez_home)),
        # miniaturas: fundo de boot de cada tema, quadro estatico com o escurecimento de boot
        ("ratimos_thumb_bootbg_classico_desc", thumb(dim_image(dither, 55))),
        ("ratimos_thumb_bootbg_noite_desc", thumb(noite_boot_static)),
        ("ratimos_thumb_bootbg_castelo_desc", thumb(dim_image(castelo_boot, 15))),
        ("ratimos_thumb_bootbg_xadrez_desc", thumb(dim_image(chess_boot, 10))),
    ]
    moon, moon_ox, moon_oy = moon_sprite()
    stars = star_table()

    parts = [
        "/*\n * GERADO por tools/generate_theme_art.py -- nao editar a mao. Reexecute o\n"
        " * script se o sketch 010 ou assets/backgrounds/bg_dither.png mudarem.\n *\n"
        " * Fundos 320x480 em RGB565 CRU (nunca indexado: a imagem indexada de tela\n"
        " * cheia passa pelo decoder do LVGL e aloca heap -- 02.1-14). Arrays de\n"
        " * uint16_t little-endian (o projeto inteiro assume little-endian).\n */\n"
        '#include "theme_art.h"\n'
    ]
    report = []
    total = 0
    for name, im in images:
        text, nbytes = emit_rgb565(name, im)
        parts.append(text)
        total += nbytes
        report.append((name, im.size, nbytes))
    text, nbytes = emit_rgb565a8("ratimos_noite_moon_desc", moon)
    parts.append(text)
    total += nbytes
    report.append(("ratimos_noite_moon_desc", moon.size, nbytes))

    parts.append("const ratimos_star_t ratimos_noite_stars[RATIMOS_NOITE_STAR_COUNT] = {")
    for (x, y, period, big, col) in stars:
        parts.append(f"    {{ {x}, {y}, {period}, {big}, 0x{col.lstrip('#')} }},")
    parts.append("};\n")
    parts.append("const ratimos_window_t ratimos_castelo_windows[RATIMOS_CASTELO_WINDOW_COUNT] = {")
    for (x, y, i) in windows:
        parts.append(f"    {{ {x}, {y}, {i} }},")
    parts.append("};\n")
    stars_bytes = STAR_COUNT * 8 + len(windows) * 6
    total += stars_bytes
    report.append(("ratimos_noite_stars + castelo_windows (tabelas)", (0, 0), stars_bytes))

    h = [
        "#ifndef RATIMOS_THEME_ART_H",
        "#define RATIMOS_THEME_ART_H",
        "",
        "/* GERADO por tools/generate_theme_art.py -- nao editar a mao. */",
        "#include <stdint.h>",
        '#include "lvgl.h"',
        "",
        "/* Noite estrelada (boot): faixas, 70 estrelas piscando e a lua. */",
        f"#define RATIMOS_NOITE_STAR_COUNT {STAR_COUNT}",
        f"#define RATIMOS_NOITE_BAND0 0x{NOITE_BANDS[0].lstrip('#')} /* y 0..159   */",
        f"#define RATIMOS_NOITE_BAND1 0x{NOITE_BANDS[1].lstrip('#')} /* y 160..319 */",
        f"#define RATIMOS_NOITE_BAND2 0x{NOITE_BANDS[2].lstrip('#')} /* y 320..479 */",
        "#define RATIMOS_NOITE_BIG_CROSS 0x6a4f8e",
        "#define RATIMOS_NOITE_STAR_PEAK 0xf5f2f8 /* fase 3 */",
        f"#define RATIMOS_NOITE_MOON_X {moon_ox}",
        f"#define RATIMOS_NOITE_MOON_Y {moon_oy}",
        "typedef struct {",
        "    int16_t x;",
        "    int16_t y;",
        "    uint16_t period_ms; /* 500 + rnd*900 */",
        "    uint8_t big;        /* ganha cruz nas fases >= 2 */",
        "    uint32_t col;       /* 0xrrggbb nas fases 1 e 2 */",
        "} ratimos_star_t;",
        "extern const ratimos_star_t ratimos_noite_stars[RATIMOS_NOITE_STAR_COUNT];",
        "extern const lv_image_dsc_t ratimos_noite_moon_desc;",
        "",
        "/* Castelo ao entardecer (boot): as janelas que apagam sao repintadas em runtime. */",
        f"#define RATIMOS_CASTELO_WINDOW_COUNT {len(windows)}",
        f"#define RATIMOS_CASTELO_WIN_W {win_w}",
        f"#define RATIMOS_CASTELO_WIN_H {win_h}",
        f"#define RATIMOS_CASTELO_WIN_ON 0x{WIN_ON.lstrip('#')}",
        f"#define RATIMOS_CASTELO_WIN_OFF 0x{WIN_OFF.lstrip('#')}",
        f"#define RATIMOS_CASTELO_WIN_TOP 0x{WIN_TOP.lstrip('#')}",
        "typedef struct {",
        "    int16_t x;",
        "    int16_t y;",
        "    uint8_t i; /* indice da janela no padrao (floor(now/800) + i) % 5 */",
        "} ratimos_window_t;",
        "extern const ratimos_window_t ratimos_castelo_windows[RATIMOS_CASTELO_WINDOW_COUNT];",
        "",
        "/* Xadrez: cores do tabuleiro (chessRender do 010). */",
        "#define RATIMOS_XADREZ_A   { 44, 14, 70 }",
        "#define RATIMOS_XADREZ_B   { 12, 4, 22 }",
        "#define RATIMOS_XADREZ_M   { 28, 9, 46 }",
        "#define RATIMOS_XADREZ_SKY0 { 6, 2, 12 }",
        "#define RATIMOS_XADREZ_SKY1 { 40, 14, 64 }",
        "#define RATIMOS_XADREZ_LINE 0x4e2277",
        "",
    ]
    for name, _im in images:
        h.append(f"extern const lv_image_dsc_t {name};")
    h += ["", "#endif", ""]

    OUT_C.write_text("\n".join(parts), encoding="utf-8")
    OUT_H.write_text("\n".join(h), encoding="utf-8")

    print(f"wrote {OUT_C}")
    print(f"wrote {OUT_H}")
    for name, size, n in report:
        print(f"  {name:50s} {size[0]:3d}x{size[1]:<3d} {n:8d} bytes")
    print(f"  {'TOTAL flash (arte dos temas)':50s}         {total:8d} bytes")


if __name__ == "__main__":
    main()
