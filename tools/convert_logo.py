#!/usr/bin/env python3
"""
Converte logo/RatimOS.png num asset de imagem LVGL compilado (RGB565,
sem alpha) para o splash de boot (D-17 / UI-SPEC "Splash image asset
pipeline"). Roda uma unica vez (ou sempre que o logo fonte mudar) e gera:

  - src/ratimos/logo_image.c  (bytes RGB565 + lv_image_dsc_t)
  - src/ratimos/logo_image.h  (declaracao extern)

Uso: python3 tools/convert_logo.py
"""
import struct
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_LOGO = REPO_ROOT / "logo" / "RatimOS.png"
OUT_C = REPO_ROOT / "src" / "ratimos" / "logo_image.c"
OUT_H = REPO_ROOT / "src" / "ratimos" / "logo_image.h"

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

    c_source = f"""/*
 * GERADO por tools/convert_logo.py a partir de logo/RatimOS.png -- nao
 * editar a mao. Reexecute o script se o logo fonte mudar.
 *
 * RGB565, {TARGET_W}x{TARGET_H}px (proporcao 3:2 preservada do PNG fonte
 * 1536x1024), sem canal alpha (D-17 / UI-SPEC "Splash image asset pipeline").
 */
#include "logo_image.h"

{emit_c_array("ratimos_logo_map", packed)}

const lv_image_dsc_t ratimos_logo_desc = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .header.w = {TARGET_W},
    .header.h = {TARGET_H},
    .header.stride = {stride},
    .data_size = sizeof(ratimos_logo_map),
    .data = ratimos_logo_map,
}};
"""

    h_source = """#ifndef RATIMOS_LOGO_IMAGE_H
#define RATIMOS_LOGO_IMAGE_H

#include "lvgl.h"

extern const lv_image_dsc_t ratimos_logo_desc;

#endif
"""

    OUT_C.write_text(c_source, encoding="utf-8")
    OUT_H.write_text(h_source, encoding="utf-8")
    print(f"wrote {OUT_C} ({len(packed)} bytes packed, {TARGET_W}x{TARGET_H})")
    print(f"wrote {OUT_H}")


if __name__ == "__main__":
    main()
