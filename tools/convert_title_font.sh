#!/usr/bin/env bash
# Converte a fonte pixel/bitmap escolhida para o tier "Heading"/"Display"
# da tipografia RatimOS (D-08) em dois arquivos C LVGL (16px e 20px),
# usando lv_font_conv (dev-tool oficial da org lvgl no npm, auditado em
# 02.1-RESEARCH.md's Package Legitimacy Audit -- versao pinada, nunca uma
# dependencia de firmware). Faixa 0x20-0x7E (ASCII imprimivel) mais o
# conjunto de simbolos acentuados PT-BR que a UI usa.
#
# Sem Node/npm disponivel? Use o fallback sem instalacao: o LVGL Font
# Converter no navegador (https://lvgl.io/tools/fontconverter) com o
# mesmo --size/--bpp/--range/--symbols abaixo.
#
# Uso: tools/convert_title_font.sh <caminho-para-o.ttf>
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "uso: $0 <caminho-para-o.ttf>" >&2
    exit 1
fi

FONT_PATH="$1"
if [ ! -f "$FONT_PATH" ]; then
    echo "arquivo de fonte nao encontrado: $FONT_PATH" >&2
    exit 1
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$REPO_ROOT/src/ratimos/fonts"
mkdir -p "$OUT_DIR"

OUT_16="$OUT_DIR/ratimos_font_title_16.c"
OUT_20="$OUT_DIR/ratimos_font_title_20.c"

# a-acute a-grave a-tilde a-circumflex e-acute e-circumflex i-acute
# o-acute o-circumflex o-tilde u-acute c-cedilha, mais as formas
# maiusculas -- exatamente o conjunto de acentos do PT-BR que os titulos
# desta fase usam (ver 02.1-UI-SPEC.md, secao Typography).
PT_BR_SYMBOLS="áàãâéêíóôõúçÁÀÃÂÉÊÍÓÔÕÚÇ"

GENERATED_HEADER() {
    local size="$1"
    local out_file="$2"
    # gera direto no arquivo final -- lv_font_conv deriva o nome do simbolo
    # C (ex.: ratimos_font_title_16) do nome do arquivo -o, entao um
    # arquivo temporario aqui renomearia o simbolo gerado incorretamente.
    npx lv_font_conv@1.5.3 \
        --font "$FONT_PATH" \
        --size "$size" \
        --bpp 4 \
        --format lvgl \
        --range 0x20-0x7E \
        --symbols "$PT_BR_SYMBOLS" \
        -o "$out_file"
    local tmp_file
    tmp_file="$(mktemp)"
    {
        printf '/*\n'
        printf ' * GERADO por tools/convert_title_font.sh a partir de %s\n' "$(basename "$FONT_PATH")"
        printf ' * (Press Start 2P, SIL Open Font License 1.1) -- nao editar a mao.\n'
        printf ' * Reexecute o script se a fonte fonte mudar. Tamanho %spx, bpp 4,\n' "$size"
        printf ' * faixa 0x20-0x7E + acentos PT-BR (D-08 / 02.1-UI-SPEC.md).\n'
        printf ' *\n'
        printf ' * Sem Node/npm? Fallback sem instalacao: lvgl.io/tools/fontconverter\n'
        printf ' * com os mesmos --size/--bpp/--range/--symbols acima.\n'
        printf ' */\n'
        cat "$out_file"
    } > "$tmp_file"
    mv "$tmp_file" "$out_file"
    # lv_font_conv emite um #ifdef LV_LVGL_H_INCLUDE_SIMPLE / #else
    # #include "lvgl/lvgl.h" / #endif no topo -- LV_LVGL_H_INCLUDE_SIMPLE
    # nunca e' definida neste projeto (que usa -DLV_CONF_INCLUDE_SIMPLE em
    # platformio.ini e "lvgl.h" direto em todo canto, ex. logo_image.h),
    # entao o ramo #else quebrava o build. Normaliza para o include unico
    # que o resto do projeto ja usa, em vez de introduzir uma nova flag de
    # build so' para este arquivo gerado.
    sed -i '/#ifdef LV_LVGL_H_INCLUDE_SIMPLE/,/#endif/c\#include "lvgl.h"' "$out_file"
    echo "$out_file"
}

GENERATED_HEADER 16 "$OUT_16"
GENERATED_HEADER 20 "$OUT_20"
