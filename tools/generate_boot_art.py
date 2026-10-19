#!/usr/bin/env python3
"""
Gera a arte dos CARREGAMENTOS da tela de boot (plano 02.1-16, sketch 010):
src/ratimos/boot_art.c e boot_art.h. Tudo em flash (const), RGB565A8 (plano
RGB565 + plano A8), COM ALFA para funcionar sobre o fundo de qualquer tema.

Fonte de verdade das formulas: o canvas de
.planning/sketches/010-temas/index.html (`BOOTS.*.draw`, `WM_GRAD`/`gradAt`,
`H_STOPS`/`hgrad`, `thumb()`), que SUBSTITUI o sketch 008 onde os dois diferem
(contorno das ameias #0d0515, etc.). O logo vem da mesma fonte e da mesma regra
de alfa de tools/convert_logo.py (alfa = 0 se max(r,g,b) < 14).

Assets gerados:
  - ratimos_boot_flag_desc              29x16   bandeira do logo (B e A)
  - ratimos_boot_wall_lit_desc          194x22  ameias acesas (A), contorno 1px
  - ratimos_boot_wall_unlit_desc        194x22  ameias apagadas (A)
  - ratimos_boot_wordmark_glow_desc     192xN   friso de brilho do letreiro (C)
  - ratimos_boot_rook_empty_desc        28x40   torre vazia (D)
  - ratimos_boot_rook_full_{0..5}_desc  28x40   torre cheia, tingida (D)
  - ratimos_boot_rook_glint_{0..5}_desc 28x40   faixa de brilho (D)
  - ratimos_boot_thumb_<estilo>_desc    40x60   miniatura do carregamento a 60%

Deterministico: rodar duas vezes gera arquivos identicos. Pillow apenas.

Uso: python3 tools/generate_boot_art.py
"""
import math
import sys
from pathlib import Path

from PIL import Image

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import convert_logo  # noqa: E402  (mesma fonte e regra de alfa do logo)

REPO_ROOT = TOOLS.parent
ROOK_SRC = REPO_ROOT / "assets" / "icons" / "system_rook.png"
OUT_C = REPO_ROOT / "src" / "ratimos" / "boot_art.c"
OUT_H = REPO_ROOT / "src" / "ratimos" / "boot_art.h"

LOGO_W, LOGO_H = 256, 171
LOGO_X, LOGO_Y = 32, 110
UNITS = 96  # RATIMOS_SPLASH_UNITS (splash.h)

# ---- constantes do sketch 010 ------------------------------------------
WM_GRAD = [
    "#460196", "#460196", "#470196", "#490299", "#5b0197", "#75046b", "#8d0355", "#9a0249",
    "#bc0127", "#b40239", "#d60321", "#e90116", "#ed0e13", "#f81711", "#fb230c", "#fb3307",
    "#fc360b", "#fb5302", "#f95d06",
]
H_STOPS = [(0x46, 0x01, 0x96), (0x9A, 0x02, 0x49), (0xE9, 0x01, 0x16), (0xF9, 0x5D, 0x06)]
MAST_COLOR = "#d9d0e6"


def hexrgb(h):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def jsround(v):
    """Math.round do JS (metade para cima), nao o arredondamento bancario."""
    return int(math.floor(v + 0.5))


def grad_at(t):
    return hexrgb(WM_GRAD[min(len(WM_GRAD) - 1, max(0, int(math.floor(t * len(WM_GRAD)))))])


def hgrad(t):
    s = min(2.999, t * 3)
    i = int(math.floor(s))
    f = s - i
    a, b = H_STOPS[i], H_STOPS[i + 1]
    return tuple(jsround(a[k] + (b[k] - a[k]) * f) for k in range(3))


# ---- imagens de entrada ----------------------------------------------------
def load_logo_rgba():
    im = Image.open(convert_logo.SRC_LOGO).convert("RGB")
    im = im.resize((convert_logo.TARGET_W, convert_logo.TARGET_H), Image.LANCZOS)
    alpha = convert_logo.alpha_plane(im)
    out = Image.new("RGBA", im.size)
    px = im.load()
    op = out.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            r, g, b = px[x, y][:3]
            op[x, y] = (r, g, b, alpha[y * im.size[0] + x])
    return out


# ---- empacotamento RGB565A8 ---------------------------------------------------
def rgb565a8(img):
    w, h = img.size
    px = img.load()
    rgb = bytearray()
    a8 = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            rgb += bytes((v & 0xFF, v >> 8))
            a8.append(a)
    return bytes(rgb) + bytes(a8)


def emit_array(name, data):
    lines = [f"static const uint8_t {name}_map[] = {{"]
    for i in range(0, len(data), 16):
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",")
    lines.append("};")
    return "\n".join(lines)


# ---- sprites --------------------------------------------------------------------
def make_flag(logo):
    return logo.crop((129, 31, 129 + 29, 31 + 16))


# Muralha da variante A (ameias): 24 colunas de 8px, 6 linhas de ameia + 14 de corpo.
MER, BODY, MW, COLS, CW = 6, 14, 8, 24, 8
WALL_H = MER + BODY  # 20
WALL_W = COLS * CW  # 192


def in_shape(x, y):
    return y >= MER or ((x // MW) % 2 == 0)


def make_wall(lit):
    out = Image.new("RGBA", (WALL_W + 2, WALL_H + 2), (0, 0, 0, 0))
    op = out.load()
    outline = hexrgb("#0d0515")
    for y in range(-1, WALL_H + 1):
        for x in range(-1, WALL_W + 1):
            inside = 0 <= x < WALL_W and 0 <= y < WALL_H and in_shape(x, y)
            if not inside:
                near = False
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    X, Y = x + dx, y + dy
                    if 0 <= X < WALL_W and 0 <= Y < WALL_H and in_shape(X, Y):
                        near = True
                        break
                if near:
                    op[x + 1, y + 1] = outline + (255,)
                continue
            gap_edge = y == 0 or (y == MER and (x // MW) % 2 == 1)
            if lit:
                col = grad_at(y / (WALL_H - 1))
                row = -1 if y < MER else (y - MER) // 5
                # `row % 2` do JS: -1 % 2 == -1 (truthy) -> deslocamento 4
                joint = ((x + (4 if (row % 2 != 0) else 0)) % CW) == CW - 1
                if gap_edge:
                    col = hexrgb("#ffb3a0")
                elif y >= MER and ((y - MER) % 5 == 4 or joint):
                    col = hexrgb("#2a0612")
            else:
                col = hexrgb("#33204a") if gap_edge else hexrgb("#1a0d29")
            op[x + 1, y + 1] = col + (255,)
    return out


def css_glow(c):
    """brightness(3.2) e depois saturate(0.4) (matriz de Filter Effects)."""
    r, g, b = (min(255.0, v * 3.2) for v in c)
    s = 0.4
    m = (
        (0.213 + 0.787 * s, 0.715 - 0.715 * s, 0.072 - 0.072 * s),
        (0.213 - 0.213 * s, 0.715 + 0.285 * s, 0.072 - 0.072 * s),
        (0.213 - 0.213 * s, 0.715 - 0.715 * s, 0.072 + 0.928 * s),
    )
    return tuple(
        max(0, min(255, jsround(m[i][0] * r + m[i][1] * g + m[i][2] * b))) for i in range(3)
    )


GLOW_X0, GLOW_X1 = 34, 226  # colunas 34..225 (WM_X0..WM_X1-1)


def make_glow(logo):
    """Friso do letreiro: as colunas 34..225 do logo com brilho. So' as linhas
    que tem alguma coisa desenhada (o resto e' alfa 0), com o deslocamento Y
    exportado como constante."""
    lp = logo.load()
    rows = [y for y in range(LOGO_H) if any(lp[x, y][3] for x in range(GLOW_X0, GLOW_X1))]
    y0, y1 = min(rows), max(rows) + 1
    out = Image.new("RGBA", (GLOW_X1 - GLOW_X0, y1 - y0), (0, 0, 0, 0))
    op = out.load()
    for y in range(y0, y1):
        for x in range(GLOW_X0, GLOW_X1):
            r, g, b, a = lp[x, y]
            if a:
                op[x - GLOW_X0, y - y0] = css_glow((r, g, b)) + (a,)
    return out, y0


def make_rooks():
    src = Image.open(ROOK_SRC).convert("RGBA")
    assert src.size == (14, 20), src.size
    sp = src.load()

    def scale2(img):
        return img.resize((28, 40), Image.NEAREST)

    empty = Image.new("RGBA", (14, 20), (0, 0, 0, 0))
    ep = empty.load()
    ecol = hexrgb("#2e1a46")
    for y in range(20):
        for x in range(14):
            if sp[x, y][3]:
                ep[x, y] = ecol + (sp[x, y][3],)

    fulls, glints = [], []
    for i in range(6):
        tint = hgrad(i / 5)
        full = Image.new("RGBA", (14, 20), (0, 0, 0, 0))
        fp = full.load()
        glint = Image.new("RGBA", (14, 20), (0, 0, 0, 0))
        gp = glint.load()
        for y in range(20):
            for x in range(14):
                r, g, b, a = sp[x, y]
                if not a:
                    continue
                f = tuple(jsround(0.6 * tint[k] + 0.4 * (r, g, b)[k]) for k in range(3))
                fp[x, y] = f + (a,)
                gp[x, y] = tuple(jsround(0.7 * 255 + 0.3 * f[k]) for k in range(3)) + (a,)
        fulls.append(scale2(full))
        glints.append(scale2(glint))
    return scale2(empty), fulls, glints


# ---- composicao dos quadros (para as miniaturas) ------------------------------------
def paste(canvas, img, dx, dy, opa=255):
    layer = img
    if opa != 255:
        layer = img.copy()
        a = layer.getchannel("A").point(lambda v: v * opa // 255)
        layer.putalpha(a)
    tmp = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    tmp.paste(layer, (dx, dy))
    canvas.alpha_composite(tmp)


def fill(canvas, x1, y1, x2, y2, col):
    canvas.paste(col + (255,), (x1, y1, x2 + 1, y2 + 1))


def frame_bandeira(logo, flag, units):
    c = Image.new("RGBA", (320, 480), (0, 0, 0, 0))
    top, ext, base = 86, 64, 47
    rise = (units * 32 // UNITS) * 2
    paste(c, logo.crop((0, 0, 256, 31)), LOGO_X, top)
    paste(c, logo.crop((125, 31, 128, 33)), LOGO_X + 125, top + 31)
    lp = logo.load()
    for k in range(3):
        fill(c, LOGO_X + 125 + k, top + 33, LOGO_X + 125 + k, top + 33 + (base - 31 - 2 + ext) - 1, lp[125 + k, 40][:3])
    paste(c, logo.crop((0, base, 256, LOGO_H)), LOGO_X, top + base + ext)
    for r in range(16):
        paste(c, flag.crop((0, r, 29, r + 1)), LOGO_X + 129 + ((r // 3) % 2), top + 31 + ext - rise + r)
    return c


def frame_ameias(logo, flag, lit_img, unlit_img, units):
    c = Image.new("RGBA", (320, 480), (0, 0, 0, 0))
    paste(c, logo, LOGO_X, LOGO_Y)
    lit = units * COLS // UNITS
    cut = lit * CW + 1
    paste(c, lit_img.crop((0, 0, cut, 22)), 65, 302)
    paste(c, unlit_img.crop((cut, 0, 194, 22)), 65 + cut, 302)
    return c


def frame_letreiro(logo, glow, glow_y0, units):
    c = Image.new("RGBA", (320, 480), (0, 0, 0, 0))
    paste(c, logo, LOGO_X, LOGO_Y, 43)
    edge = 34 + ((units * 193) // (UNITS * 4)) * 4
    if edge > 34:
        paste(c, logo.crop((0, 0, edge, LOGO_H)), LOGO_X, LOGO_Y)
    if edge < 226:
        paste(c, glow.crop((edge - GLOW_X0, 0, edge - GLOW_X0 + 2, glow.size[1])), LOGO_X + edge, LOGO_Y + glow_y0)
    return c


def frame_torres(logo, empty, fulls, glints, units):
    c = Image.new("RGBA", (320, 480), (0, 0, 0, 0))
    paste(c, logo, LOGO_X, LOGO_Y)
    rows = units * 120 // UNITS
    for i in range(6):
        f = max(0, min(20, rows - 20 * i))
        hop = 2 if (f == 20 and rows - 20 * (i + 1) < 4 and rows < 120) else 0
        x = 58 + i * 35
        y = 299 - hop
        split = 40 - 2 * f
        if split > 0:
            paste(c, empty.crop((0, 0, 28, split)), x, y)
        if f > 0:
            band = 2 if f < 20 else 0
            if band:
                paste(c, glints[i].crop((0, split, 28, split + 2)), x, y + split)
            paste(c, fulls[i].crop((0, split + band, 28, 40)), x, y + split + band)
    return c


def thumb(frame):
    return frame.resize((40, 60), Image.LANCZOS)


# ---- emissao ---------------------------------------------------------------------
def main():
    logo = load_logo_rgba()
    flag = make_flag(logo)
    wall_lit, wall_unlit = make_wall(True), make_wall(False)
    glow, glow_y0 = make_glow(logo)
    empty, fulls, glints = make_rooks()

    t_units = UNITS * 6 // 10
    thumbs = {
        "bandeira": thumb(frame_bandeira(logo, flag, t_units)),
        "ameias": thumb(frame_ameias(logo, flag, wall_lit, wall_unlit, t_units)),
        "letreiro": thumb(frame_letreiro(logo, glow, glow_y0, t_units)),
        "torres": thumb(frame_torres(logo, empty, fulls, glints, t_units)),
    }

    assets = [
        ("ratimos_boot_flag_desc", flag),
        ("ratimos_boot_wall_lit_desc", wall_lit),
        ("ratimos_boot_wall_unlit_desc", wall_unlit),
        ("ratimos_boot_wordmark_glow_desc", glow),
        ("ratimos_boot_rook_empty_desc", empty),
    ]
    assets += [(f"ratimos_boot_rook_full_{i}_desc", fulls[i]) for i in range(6)]
    assets += [(f"ratimos_boot_rook_glint_{i}_desc", glints[i]) for i in range(6)]
    assets += [(f"ratimos_boot_thumb_{k}_desc", v) for k, v in thumbs.items()]

    c_parts = [
        "/*\n * GERADO por tools/generate_boot_art.py -- nao editar a mao. Reexecute o\n"
        " * script se o logo, o icone da torre ou a arte do sketch 010 mudarem.\n *\n"
        " * Sprites RGB565A8 (plano RGB565 + plano A8) com alfa, em flash, para os 4\n"
        " * carregamentos da tela de boot sobre o fundo de qualquer tema (plano\n"
        " * 02.1-16). Formulas: .planning/sketches/010-temas/index.html.\n */\n"
        '#include "boot_art.h"\n'
    ]
    total = 0
    report = []
    for name, img in assets:
        data = rgb565a8(img)
        total += len(data)
        report.append((name, img.size, len(data)))
        base = name[: -len("_desc")]
        c_parts.append(emit_array(base, data))
        w, h = img.size
        c_parts.append(
            f"const lv_image_dsc_t {name} = {{\n"
            f"    .header.magic = LV_IMAGE_HEADER_MAGIC,\n"
            f"    .header.cf = LV_COLOR_FORMAT_RGB565A8,\n"
            f"    .header.w = {w},\n"
            f"    .header.h = {h},\n"
            f"    .header.stride = {w * 2},\n"
            f"    .data_size = sizeof({base}_map),\n"
            f"    .data = {base}_map,\n"
            f"}};\n"
        )

    h_lines = [
        "#ifndef RATIMOS_BOOT_ART_H",
        "#define RATIMOS_BOOT_ART_H",
        "",
        "/* GERADO por tools/generate_boot_art.py -- nao editar a mao. */",
        '#include "lvgl.h"',
        "",
        "/* Mastro da variante ameias (sketch 010). */",
        f"#define RATIMOS_BOOT_MAST_HEX 0x{MAST_COLOR.lstrip('#')}",
        "/* Linha do logo onde o sprite do brilho do letreiro comeca (o sprite so' tem as linhas com conteudo). */",
        f"#define RATIMOS_BOOT_GLOW_Y0 {glow_y0}",
        f"#define RATIMOS_BOOT_GLOW_X0 {GLOW_X0}",
        "",
    ]
    for name, _ in assets:
        h_lines.append(f"extern const lv_image_dsc_t {name};")
    h_lines += ["", "#endif", ""]

    OUT_C.write_text("\n".join(c_parts), encoding="utf-8")
    OUT_H.write_text("\n".join(h_lines), encoding="utf-8")

    print(f"wrote {OUT_C}")
    print(f"wrote {OUT_H}")
    for name, size, n in report:
        print(f"  {name:42s} {size[0]:3d}x{size[1]:<3d} {n:7d} bytes")
    print(f"  {'TOTAL flash (arte dos carregamentos)':42s}         {total:7d} bytes")


if __name__ == "__main__":
    main()
