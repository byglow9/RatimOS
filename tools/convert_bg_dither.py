#!/usr/bin/env python3
"""
Converte assets/backgrounds/bg_dither.png (gerado por
tools/generate_bg_dither.py) num asset de imagem LVGL compilado em RGB565
(sem paleta) -- mesmo padrao de tools/convert_logo.py, NAO o pipeline
indexado (LV_COLOR_FORMAT_I4) de tools/convert_images.py.

Por que RGB565 aqui e nao I4 (como icons.c/progress_images.c usam)?
Diagnostico real (plano 02.1-09, apos verificacao humana no simulador
SDL2): com este projeto's LV_BIN_DECODER_RAM_LOAD desligado (padrao do
LVGL, nao definido em lv_conf.h), o decoder de imagem indexada do LVGL
(lv_bin_decoder.c:decode_indexed()) NUNCA produz um buffer decodificado
completo para uma imagem LV_IMAGE_SRC_VARIABLE -- o proprio comentario do
LVGL vendorizado diz "Convert to ARGB8888, since sw renderer cannot render
it directly even it's in RAM", codigo que so' roda quando
LV_BIN_DECODER_RAM_LOAD=1. Sem isso, o LVGL cai no caminho de decode
"em pedacos" (lv_image_decoder_get_area(), lv_draw_image.c:
img_decode_and_draw()) -- que funciona bem pro caminho de blit 1:1 (por
isso os icones I4 desta fase renderizam certo, nenhum usa
LV_IMAGE_ALIGN_STRETCH), mas nao e' compativel com o caminho de desenho
TRANSFORMADO (rotacao/escala) que ratimos_theme_apply_screen() usa pra
esticar este fundo de 80x120 pra 320x480 (LV_IMAGE_ALIGN_STRETCH seta
scale_x/scale_y != LV_SCALE_NONE, ver lv_image.c). O resultado observado
no simulador real foi ruido visual/estatico, nao o gradiente pretendido --
os bytes do array C em si estavam 100% corretos (confirmados por
decodificacao manual contra a paleta), o bug e' especificamente na
combinacao decode-indexado-em-pedacos + escala do LVGL, nao nos dados.

RGB565 nao e' um formato indexado (sem paleta, sem decode_indexed()
nenhum) -- 2 bytes/pixel, cada pixel decodificado diretamente pelo
caminho de blit padrao do LVGL, que ja suporta corretamente o desenho
transformado (mesmo formato usado por src/ratimos/logo_image.c, o logo
da splash, ja comprovado funcionando em producao). Custo de RAM pra
80x120 RGB565: 80*120*2 = 19200 bytes (~19KB) -- praticamente identico
ao orcamento de decode que a fonte 80x120 (nao 320x480) ja visava desde
o inicio (ver generate_bg_dither.py), so' que agora alcancado com o
formato certo em vez do indexado que o LVGL nao consegue esticar aqui.
NUNCA reverter isto pra I4 "pra economizar espaco" sem antes resolver o
bug de decode acima -- indexado volta a corromper a tela inteira assim
que qualquer image_align diferente de 1:1 for usado.

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
 * RGB565, {w}x{h}px, sem paleta/canal alpha -- NAO o formato indexado
 * LV_COLOR_FORMAT_I4 que icons.c/progress_images.c usam. Ver o docstring
 * de tools/convert_bg_dither.py para o porque (bug real de renderizacao
 * do LVGL ao combinar decode indexado-em-pedacos com escala/stretch).
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
