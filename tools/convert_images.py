#!/usr/bin/env python3
"""
Converte um conjunto de PNGs de pixel art num asset de imagem LVGL compilado
(LV_COLOR_FORMAT_I4, paleta indexada de ate 16 cores) -- generaliza
tools/convert_logo.py (que usa RGB565 para o logo unico) para qualquer
conjunto de icones/sprites orientado por manifest, de forma que o mesmo
script sirva tanto para o icon set desta fase (VISUAL-01) quanto para a
arte de progressao castelo/jardim da fase 02.1-03.

O manifest e' um JSON com a forma:
{
  "symbol_prefix": "ratimos_icon",
  "images": [
    { "id": "home_jogos", "file": "home_jogos.png", "w": 32, "h": 32 },
    ...
  ]
}
`file` e' relativo ao diretorio do proprio manifest.

Uso:
  python3 tools/convert_images.py --manifest assets/icons/manifest.json \
      --out-c src/ratimos/icons.c --out-h src/ratimos/icons.h \
      --guard RATIMOS_ICONS_H
"""
import argparse
import json
import sys
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent

MAX_PALETTE = 16


def quantize_to_indexed(im: Image.Image):
    """Quantiza `im` (RGBA) para no maximo 16 cores + indice 0 reservado
    para transparencia (mesmo que a imagem nao use alpha). Retorna
    (palette, indices) onde `palette` e' uma lista de ate 16 tuplas
    (r, g, b, a) e `indices` e' uma lista de linhas de indices de paleta
    (um indice por pixel, row-major, na ordem em que cada cor apareceu
    pela primeira vez -- deterministico para o mesmo arquivo fonte)."""
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()

    palette = [(0, 0, 0, 0)]  # indice 0: sempre reservado para transparencia
    color_to_index = {(0, 0, 0, 0): 0}
    indices = []

    for y in range(h):
        row = []
        for x in range(w):
            r, g, b, a = px[x, y]
            key = (0, 0, 0, 0) if a < 128 else (r, g, b, 255)
            idx = color_to_index.get(key)
            if idx is None:
                if len(palette) >= MAX_PALETTE:
                    raise ValueError(
                        f"imagem usa mais de {MAX_PALETTE} cores distintas "
                        f"(incluindo transparencia) -- reduza a paleta fonte"
                    )
                idx = len(palette)
                color_to_index[key] = idx
                palette.append(key)
            row.append(idx)
        indices.append(row)

    return palette, indices


def pack_i4(palette, indices):
    """Empacota a paleta (BGRA, 4 bytes/entrada, sempre 16 entradas -- as
    faltantes preenchidas com preto-transparente) seguida dos indices de
    pixel, 2 por byte (nibble alto = pixel par, nibble baixo = pixel
    impar), cada linha alinhada a um byte inteiro -- exatamente o layout
    que LV_COLOR_FORMAT_I4 espera (confirmado contra
    examples/assets/img_cogwheel_indexed16.c e
    src/libs/bin_decoder/lv_bin_decoder.c:decode_indexed_line() do LVGL
    vendorizado). Retorna (bytes_empacotados, stride_em_bytes)."""
    out = bytearray()
    fixed_palette = list(palette) + [(0, 0, 0, 0)] * (MAX_PALETTE - len(palette))
    for r, g, b, a in fixed_palette:
        out += bytes([b, g, r, a])  # LVGL espera BGRA por entrada de paleta

    h = len(indices)
    w = len(indices[0]) if h else 0
    stride = (w + 1) // 2

    for row in indices:
        packed_row = bytearray(stride)
        for x, idx in enumerate(row):
            byte_i = x // 2
            if x % 2 == 0:
                packed_row[byte_i] |= (idx & 0x0F) << 4
            else:
                packed_row[byte_i] |= idx & 0x0F
        out += packed_row

    return bytes(out), stride


def emit_c_array(name: str, data: bytes) -> str:
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--manifest", required=True, type=Path)
    ap.add_argument("--out-c", required=True, type=Path)
    ap.add_argument("--out-h", required=True, type=Path)
    ap.add_argument("--guard", required=True)
    args = ap.parse_args()

    manifest_path = args.manifest.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    prefix = manifest["symbol_prefix"]
    images = manifest["images"]
    manifest_dir = manifest_path.parent

    c_chunks = []
    h_chunks = []
    table_entries = []

    for entry in images:
        img_id = entry["id"]
        img_path = manifest_dir / entry["file"]
        w = entry["w"]
        img_h = entry["h"]

        im = Image.open(img_path)
        if im.size != (w, img_h):
            raise ValueError(f"{img_path}: tamanho {im.size} != manifest ({w}x{img_h})")

        palette, indices = quantize_to_indexed(im)
        packed, stride = pack_i4(palette, indices)

        map_name = f"{prefix}_{img_id}_map"
        desc_name = f"{prefix}_{img_id}_desc"

        c_chunks.append(emit_c_array(map_name, packed))
        c_chunks.append(f"""
const lv_image_dsc_t {desc_name} = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_I4,
    .header.w = {w},
    .header.h = {img_h},
    .header.stride = {stride},
    .data_size = sizeof({map_name}),
    .data = {map_name},
}};
""")
        h_chunks.append(f"extern const lv_image_dsc_t {desc_name};")
        table_entries.append((img_id, desc_name))

    table_lines = [
        "typedef struct {",
        "    const char * id;",
        "    const lv_image_dsc_t * desc;",
        f"}} {prefix}_entry_t;",
        "",
        f"static const {prefix}_entry_t {prefix}_table[] = {{",
    ]
    for img_id, desc_name in table_entries:
        table_lines.append(f'    {{ "{img_id}", &{desc_name} }},')
    table_lines.append("};")

    lookup_fn = f"""
/* Retorna o descritor da PRIMEIRA entrada da tabela cujo id bate com
 * `id` -- um id duplicado no manifest.json resolve deterministicamente
 * para a entrada mais antiga (ordem de declaracao no manifest). Retorna
 * NULL para id NULL ou sem nenhum match. */
const lv_image_dsc_t * {prefix}_by_id(const char * id)
{{
    if (!id) return NULL;
    for (size_t i = 0; i < sizeof({prefix}_table) / sizeof({prefix}_table[0]); i++) {{
        if (strcmp({prefix}_table[i].id, id) == 0) {{
            return {prefix}_table[i].desc;
        }}
    }}
    return NULL;
}}
"""

    rel_manifest = manifest_path.relative_to(REPO_ROOT)
    header_comment = f"""/*
 * GERADO por tools/convert_images.py a partir de {rel_manifest} -- nao
 * editar a mao. Reexecute o script se a arte fonte mudar.
 *
 * Formato de imagem indexado de 4 bits/pixel do LVGL (paleta de ate 16
 * cores) -- ver "Icon & Title Font Asset Pipeline" em 02.1-UI-SPEC.md.
 */
"""

    c_source = (
        header_comment
        + f'#include "{args.out_h.name}"\n#include <string.h>\n\n'
        + "\n".join(c_chunks)
        + "\n"
        + "\n".join(table_lines)
        + "\n"
        + lookup_fn
    )

    h_source = (
        header_comment
        + f"#ifndef {args.guard}\n#define {args.guard}\n\n"
        + '#include "lvgl.h"\n#include <stddef.h>\n\n'
        + "\n".join(h_chunks)
        + f"\n\nconst lv_image_dsc_t * {prefix}_by_id(const char * id);\n\n#endif\n"
    )

    args.out_c.parent.mkdir(parents=True, exist_ok=True)
    args.out_h.parent.mkdir(parents=True, exist_ok=True)
    args.out_c.write_text(c_source, encoding="utf-8")
    args.out_h.write_text(h_source, encoding="utf-8")
    print(f"wrote {args.out_c} ({len(images)} imagens)")
    print(f"wrote {args.out_h}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
