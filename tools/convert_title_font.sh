#!/usr/bin/env bash
# Converte uma fonte TTF em arquivos C LVGL (um por tamanho), usando
# lv_font_conv (dev-tool oficial da org lvgl no npm, auditado em
# 02.1-RESEARCH.md's Package Legitimacy Audit -- versao pinada, nunca uma
# dependencia de firmware). Faixa 0x20-0x7E (ASCII imprimivel) mais o
# conjunto de acentos PT-BR que a UI usa e o ponto medio "·" (U+00B7) das
# descricoes do sketch 003-C ("grade 9x9 · facil, medio, dificil").
#
# Sem Node/npm disponivel? Use o fallback sem instalacao: o LVGL Font
# Converter no navegador (https://lvgl.io/tools/fontconverter) com o
# mesmo --size/--bpp/--range/--symbols abaixo.
#
# Uso:
#   tools/convert_title_font.sh <fonte.ttf>
#       uso original (D-08): gera ratimos_font_title_16 e _20.
#   tools/convert_title_font.sh <fonte.ttf> <prefixo> <tamanho>... [--fallback <simbolo_lvgl>]
#       gera <prefixo>_<tamanho>.c para cada tamanho. --fallback grava
#       `.fallback = &<simbolo_lvgl>` no descritor gerado (glifos ausentes,
#       ex. LV_SYMBOL_*, sao buscados nessa fonte).
#
# Fontes do sketch 003-C (plano 02.1-14):
#   tools/convert_title_font.sh assets/fonts/source/JetBrainsMono-Regular.ttf \
#       ratimos_font_mono 10 11 12 --fallback lv_font_montserrat_14
#   tools/convert_title_font.sh assets/fonts/source/PressStart2P-Regular.ttf \
#       ratimos_font_title 8
set -euo pipefail

usage() {
    echo "uso: $0 <fonte.ttf> [<prefixo> <tamanho>... [--fallback <simbolo_lvgl>]]" >&2
    exit 1
}

if [ "$#" -lt 1 ]; then
    usage
fi

FONT_PATH="$1"
shift
if [ ! -f "$FONT_PATH" ]; then
    echo "arquivo de fonte nao encontrado: $FONT_PATH" >&2
    exit 1
fi

PREFIX="ratimos_font_title"
SIZES=()
FALLBACK=""

if [ "$#" -eq 0 ]; then
    # Uso original: titulo 16/20, sem fallback.
    SIZES=(16 20)
else
    PREFIX="$1"
    shift
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --fallback)
                [ "$#" -ge 2 ] || usage
                FALLBACK="$2"
                shift 2
                ;;
            *)
                case "$1" in
                    ''|*[!0-9]*) echo "tamanho invalido: $1" >&2; usage ;;
                esac
                SIZES+=("$1")
                shift
                ;;
        esac
    done
    [ "${#SIZES[@]}" -gt 0 ] || usage
fi

case "$PREFIX" in
    ''|*[!a-z0-9_]*) echo "prefixo invalido (use [a-z0-9_]): $PREFIX" >&2; exit 1 ;;
esac

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$REPO_ROOT/src/ratimos/fonts"
mkdir -p "$OUT_DIR"

# a-acute a-grave a-tilde a-circumflex e-acute e-circumflex i-acute
# o-acute o-circumflex o-tilde u-acute c-cedilha, mais as formas
# maiusculas -- o conjunto de acentos do PT-BR que a UI usa (ver
# 02.1-UI-SPEC.md, secao Typography) -- mais o ponto medio U+00B7.
PT_BR_SYMBOLS="áàãâéêíóôõúçÁÀÃÂÉÊÍÓÔÕÚÇ·"

FONT_NAME="$(basename "$FONT_PATH")"

convert_one() {
    local size="$1"
    local out_file="$OUT_DIR/${PREFIX}_${size}.c"
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
        printf ' * GERADO por tools/convert_title_font.sh a partir de %s\n' "$FONT_NAME"
        printf ' * (SIL Open Font License 1.1) -- nao editar a mao.\n'
        printf ' * Reexecute o script se a fonte fonte mudar. Tamanho %spx, bpp 4,\n' "$size"
        printf ' * faixa 0x20-0x7E + acentos PT-BR + U+00B7 (D-08 / 02.1-14).\n'
        if [ -n "$FALLBACK" ]; then
            printf ' * Glifos ausentes (ex. LV_SYMBOL_*) caem em %s.\n' "$FALLBACK"
        fi
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
    # Caminho absoluto da maquina de quem gerou nao entra no repo.
    sed -i "s#-o [^ ]*/src/ratimos/fonts/#-o src/ratimos/fonts/#" "$out_file"
    if [ -n "$FALLBACK" ]; then
        if ! grep -q '    .fallback = NULL,' "$out_file"; then
            echo "campo .fallback nao encontrado em $out_file" >&2
            exit 1
        fi
        sed -i "s#    .fallback = NULL,#    .fallback = \&${FALLBACK},#" "$out_file"
    fi
    echo "$out_file"
}

for size in "${SIZES[@]}"; do
    convert_one "$size"
done
