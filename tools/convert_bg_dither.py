#!/usr/bin/env python3
"""
Converte assets/backgrounds/bg_dither.png (gerado por
tools/generate_bg_dither.py) num asset de imagem LVGL compilado em RGB565
(sem paleta) -- mesmo padrao de tools/convert_logo.py, NAO o pipeline
indexado (LV_COLOR_FORMAT_I4) de tools/convert_images.py.

Plano 02.1-14: o PNG agora e' 320x480 (o frame inteiro) e theme.c o
desenha 1:1, sem stretch. Antes era 80x120 esticado 4x em runtime, o que
transformava cada scanline de 1px numa faixa de 4px (divergia do sketch
001-C). O limite de 80x120 vinha do medo de um buffer de decode de
~300KB no heap do LVGL -- esse risco era do formato INDEXADO (I4), cujo
decode gera um buffer. RGB565 cru num array `const` e' desenhado direto
da flash pelo caminho de blit padrao do LVGL: nenhum buffer de decode,
nenhum byte do heap (medido no SUMMARY do 02.1-14 com lv_mem_monitor).
O custo vai pra flash: 320*480*2 = 307200 bytes, folgado no slot OTA de
~6MB.

Por que nao I4 (como icons.c/progress_images.c)? Com
LV_BIN_DECODER_RAM_LOAD desligado (padrao), o decoder indexado do LVGL
nunca produz um buffer completo para LV_IMAGE_SRC_VARIABLE e cai no
decode "em pedacos", incompativel com desenho transformado (diagnostico
do plano 02.1-09, que viu ruido/estatico no simulador). Mesmo sem stretch
hoje, RGB565 e' o formato que o display usa nativamente (LV_COLOR_DEPTH
16) e nao gasta decode nenhum. NUNCA reverter isto pra I4 sem medir.

Uso: python3 tools/convert_bg_dither.py
"""
import struct
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_PNG = REPO_ROOT / "assets" / "backgrounds" / "bg_dither.png"
OUT_C = REPO_ROOT / "src" / "ratimos" / "bg_images.c"
OUT_H = REPO_ROOT / "src" / "ratimos" / "bg_images.h"


def rgb565_bytes(im: Image.Image) -> bytes:
    """Empacota cada pixel em RGB565 little-endian, 2 bytes por pixel,
    linha a linha (row-major), sem canal alpha -- identico a
    tools/convert_logo.py's rgb565_bytes()."""
    out = bytearray()
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            out += struct.pack("<H", value)
    return bytes(out)


def emit_c_array(name: str, data: bytes) -> str:
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    im = Image.open(SRC_PNG).convert("RGB")
    w, h = im.size

    packed = rgb565_bytes(im)
    stride = w * 2

    c_source = f"""/*
 * GERADO por tools/convert_bg_dither.py a partir de
 * assets/backgrounds/bg_dither.png -- nao editar a mao. Reexecute o script
 * se o PNG fonte mudar (tools/generate_bg_dither.py).
 *
 * RGB565, {w}x{h}px (frame inteiro, desenhado 1:1 direto da flash), sem
 * paleta/canal alpha -- NAO o formato indexado LV_COLOR_FORMAT_I4 que
 * icons.c/progress_images.c usam. Ver o docstring de
 * tools/convert_bg_dither.py para o porque.
 */
#include "bg_images.h"

{emit_c_array("ratimos_bg_dither_map", packed)}

const lv_image_dsc_t ratimos_bg_dither_desc = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = {w},
    .header.h = {h},
    .header.stride = {stride},
    .data_size = sizeof(ratimos_bg_dither_map),
    .data = ratimos_bg_dither_map,
}};
"""

    h_source = """/*
 * GERADO por tools/convert_bg_dither.py a partir de
 * assets/backgrounds/bg_dither.png -- nao editar a mao.
 *
 * RGB565 (nao indexado) -- ver o comentario em bg_images.c ou o docstring
 * de tools/convert_bg_dither.py para o porque.
 */
#ifndef RATIMOS_BG_IMAGES_H
#define RATIMOS_BG_IMAGES_H

#include "lvgl.h"

extern const lv_image_dsc_t ratimos_bg_dither_desc;

#endif
"""

    OUT_C.write_text(c_source, encoding="utf-8")
    OUT_H.write_text(h_source, encoding="utf-8")
    print(f"wrote {OUT_C} ({len(packed)} bytes packed, {w}x{h})")
    print(f"wrote {OUT_H}")


if __name__ == "__main__":
    main()
