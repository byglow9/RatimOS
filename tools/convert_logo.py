#!/usr/bin/env python3
"""
Converte logo/RatimOS.png em assets de imagem LVGL compilados para o splash
de boot (D-17 / UI-SPEC "Splash image asset pipeline"). Roda uma unica vez
(ou sempre que o logo fonte mudar) e gera:

  - src/ratimos/logo_image.c  (ratimos_logo_desc: RGB565 opaco 256x171, e
                               ratimos_logo_alpha_desc: RGB565A8 256x171)
  - src/ratimos/logo_image.h  (declaracoes extern)

Logo com alfa (plano 02.1-16, sketch 009/010): o boot agora desenha sobre o
fundo do TEMA, entao o logo nao pode carregar a caixa preta opaca. A regra do
sketch 009 e: alfa = 0 se max(r,g,b) < 14, senao 255 (cor igual ao logo). O
script FALHA se o alfa gerado divergir de
.planning/sketches/009-boot-fundo/logo_256_alpha.png.

Uso: python3 tools/convert_logo.py
"""
import struct
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_LOGO = REPO_ROOT / "logo" / "RatimOS.png"
OUT_C = REPO_ROOT / "src" / "ratimos" / "logo_image.c"
OUT_H = REPO_ROOT / "src" / "ratimos" / "logo_image.h"

SKETCH_ALPHA = REPO_ROOT / ".planning" / "sketches" / "009-boot-fundo" / "logo_256_alpha.png"
ALPHA_THRESHOLD = 14  # alfa = 0 se max(r,g,b) < 14 (sketch 009)

TARGET_W = 256
TARGET_H = 171  # preserva a proporcao 3:2 do PNG fonte (1536x1024)


def rgb565_bytes(im: Image.Image) -> bytes:
    """Empacota cada pixel em RGB565 little-endian, 2 bytes por pixel,
    linha a linha (row-major), sem canal alpha (D-17 / UI-SPEC)."""
    out = bytearray()
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            out += struct.pack("<H", value)
    return bytes(out)


def alpha_plane(im: Image.Image) -> bytes:
    """Plano A8 (1 byte por pixel, row-major) pela regra do sketch 009."""
    out = bytearray()
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            out.append(0 if max(r, g, b) < ALPHA_THRESHOLD else 255)
    return bytes(out)


def verify_against_sketch(im: Image.Image, alpha: bytes) -> None:
    """Confere alfa E cor (onde o alfa e 255) contra o PNG do sketch 009."""
    ref = Image.open(SKETCH_ALPHA).convert("RGBA")
    if ref.size != im.size:
        raise SystemExit(f"FALHA: tamanho {ref.size} do sketch 009 != {im.size}")
    rpx = ref.load()
    mpx = im.load()
    w, h = im.size
    bad_alpha = 0
    bad_color = 0
    for y in range(h):
        for x in range(w):
            ra = rpx[x, y][3]
            ga = alpha[y * w + x]
            if ra != ga:
                bad_alpha += 1
            elif ga == 255 and rpx[x, y][:3] != mpx[x, y][:3]:
                bad_color += 1
    if bad_alpha or bad_color:
        raise SystemExit(
            f"FALHA: alfa diverge do sketch 009 ({bad_alpha} pixels de alfa, "
            f"{bad_color} de cor)"
        )


def emit_c_array(name: str, data: bytes) -> str:
    lines = []
    lines.append(f"static const uint8_t {name}[] = {{")
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    im = Image.open(SRC_LOGO).convert("RGB")
    im = im.resize((TARGET_W, TARGET_H), Image.LANCZOS)

    packed = rgb565_bytes(im)
    stride = TARGET_W * 2

    alpha = alpha_plane(im)
    verify_against_sketch(im, alpha)
    # RGB565A8 do LVGL 9.5: plano RGB565 (stride = w*2) seguido do plano A8
    # (stride = w) -- data_size = w*h*3.
    packed_alpha = packed + alpha

    c_source = f"""/*
 * GERADO por tools/convert_logo.py a partir de logo/RatimOS.png -- nao
 * editar a mao. Reexecute o script se o logo fonte mudar.
 *
 * ratimos_logo_desc: RGB565, {TARGET_W}x{TARGET_H}px (proporcao 3:2 preservada
 * do PNG fonte 1536x1024), sem canal alpha (D-17 / UI-SPEC "Splash image
 * asset pipeline").
 *
 * ratimos_logo_alpha_desc: mesmas cores em RGB565A8 (plano RGB565 + plano A8),
 * alfa = 0 onde max(r,g,b) < {ALPHA_THRESHOLD} (regra do sketch 009, conferida
 * pixel a pixel contra logo_256_alpha.png), para o boot desenhar sobre o
 * fundo de qualquer tema sem caixa preta (plano 02.1-16).
 */
#include "logo_image.h"

/* Um unico array: plano RGB565 seguido do plano A8. ratimos_logo_desc (opaco)
 * le so' o 1o plano do MESMO buffer, entao nao ha dado duplicado na flash. */
{emit_c_array("ratimos_logo_alpha_map", packed_alpha)}

const lv_image_dsc_t ratimos_logo_desc = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = {TARGET_W},
    .header.h = {TARGET_H},
    .header.stride = {stride},
    .data_size = {len(packed)},
    .data = ratimos_logo_alpha_map,
}};

const lv_image_dsc_t ratimos_logo_alpha_desc = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565A8,
    .header.w = {TARGET_W},
    .header.h = {TARGET_H},
    .header.stride = {stride},
    .data_size = sizeof(ratimos_logo_alpha_map),
    .data = ratimos_logo_alpha_map,
}};
"""

    h_source = """#ifndef RATIMOS_LOGO_IMAGE_H
#define RATIMOS_LOGO_IMAGE_H

#include "lvgl.h"

extern const lv_image_dsc_t ratimos_logo_desc;
/* RGB565A8 256x171: mesmo logo com alfa (sketch 009), para o boot tematico. */
extern const lv_image_dsc_t ratimos_logo_alpha_desc;

#endif
"""

    OUT_C.write_text(c_source, encoding="utf-8")
    OUT_H.write_text(h_source, encoding="utf-8")
    print(f"wrote {OUT_C} ({len(packed)} + {len(packed_alpha)} bytes packed, {TARGET_W}x{TARGET_H})")
    print("alpha conferido contra o sketch 009: 0 diferencas")
    print(f"wrote {OUT_H}")


if __name__ == "__main__":
    main()
