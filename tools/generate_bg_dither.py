#!/usr/bin/env python3
"""
Gera o PNG fonte 80x120 do fundo gradiente ditherizado
(assets/backgrounds/bg_dither.png) de forma procedural (Pillow), no mesmo
espirito de tools/generate_icon_art.py/generate_progress_art.py: um script
project-owned, deterministico e reproduzivel a partir do codigo fonte, sem
depender de nenhum binario nao versionado nem de um editor externo.

Direcao validada em .claude/skills/sketch-findings-ratimos/references/
fundo-e-ambiente.md (sketch 001, variante C vencedora): gradiente vertical
de 4 stops (roxo escuro -> magenta -> rosa/vermelho -> laranja), dithering
ORDENADO (matriz Bayer 4x4) escolhendo entre as DUAS cores de stop mais
proximas por pixel -- nunca um blend continuo -- mais scanlines horizontais
sutis (1 a cada 3 linhas, ~12%) escurecendo a cor ja escolhida naquele
pixel.

Gerado em 80x120 (NAO 320x480) de proposito -- decisao estrutural de
seguranca de RAM, nao um atalho: o LVGL decodifica uma imagem indexada na
resolucao FONTE independente do tamanho de destino em tela, entao uma
fonte 80x120 mantem o buffer de decode em ~19KB, enquanto uma fonte
320x480 ingenua precisaria de ~300KB -- mais da metade do LV_MEM_SIZE de
512KB deste projeto (ja levantado uma vez na Fase 1 especificamente por
causa de uma classe de bug de esgotamento de heap do LVGL; nao reabrir
esse risco). O tamanho final 320x480 em tela e' alcancado via
image-rendering scale/stretch do LVGL em tempo de execucao (theme.c), nao
gravando um bitmap maior aqui.

O numero total de cores distintas fica bem abaixo do orcamento de 16 cores
de tools/convert_images.py's LV_COLOR_FORMAT_I4 (4 stops + 4 variantes
escurecidas de scanline = 8 cores opacas, + 1 indice reservado pra
transparencia = 9 de 16) -- rodar este script duas vezes produz um PNG
byte-identico (nenhuma aleatoriedade, matriz Bayer fixa, paleta fixa).

Uso: python3 tools/generate_bg_dither.py
"""
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_PATH = REPO_ROOT / "assets" / "backgrounds" / "bg_dither.png"

W = 80
H = 120

# Paleta dos 4 stops (vertical, topo -> base), hex exatos de
# fundo-e-ambiente.md.
STOPS = [
    (0x1c, 0x0a, 0x3d),  # 0%
    (0x7a, 0x15, 0x60),  # ~42%
    (0xc8, 0x1f, 0x4c),  # ~68%
    (0xe8, 0x63, 0x0f),  # 100%
]
STOP_POSITIONS = [0.0, 0.42, 0.68, 1.0]

# Matriz de dithering ordenado Bayer 4x4 classica (valores 0..15).
BAYER_4X4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]

# Scanlines horizontais sutis: escurece a cor escolhida em ~12% a cada 3a
# linha -- validado como o teto seguro em fundo-e-ambiente.md ("mais que
# isso arrisca 'sujar' texto pequeno").
SCANLINE_EVERY = 3
SCANLINE_DARKEN = 0.12


def darken(rgb, amount):
    return tuple(max(0, int(round(c * (1.0 - amount)))) for c in rgb)


def segment_for(t: float):
    """Retorna (cor_perto, cor_longe, t_local) para o segmento de gradiente
    (par de stops adjacentes) que contem a posicao vertical normalizada
    `t` (0..1) -- t_local e' a posicao de t dentro desse segmento (0..1),
    usada pelo dithering ordenado pra escolher entre as duas cores."""
    for i in range(len(STOP_POSITIONS) - 1):
        p0, p1 = STOP_POSITIONS[i], STOP_POSITIONS[i + 1]
        if t <= p1 or i == len(STOP_POSITIONS) - 2:
            span = p1 - p0
            t_local = (t - p0) / span if span > 0 else 0.0
            t_local = min(max(t_local, 0.0), 1.0)
            return STOPS[i], STOPS[i + 1], t_local
    return STOPS[-1], STOPS[-1], 1.0


def generate() -> Image.Image:
    im = Image.new("RGB", (W, H))
    px = im.load()

    for y in range(H):
        t = y / (H - 1) if H > 1 else 0.0
        near, far, t_local = segment_for(t)
        scanline_row = (y % SCANLINE_EVERY) == 0

        for x in range(W):
            # Threshold Bayer normalizado em (0, 1) -- ordenado, nao
            # aleatorio, garante determinismo pixel-a-pixel.
            threshold = (BAYER_4X4[y % 4][x % 4] + 0.5) / 16.0
            color = far if t_local > threshold else near

            if scanline_row:
                color = darken(color, SCANLINE_DARKEN)

            px[x, y] = color

    return im


def main() -> None:
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    im = generate()
    im.save(OUT_PATH)
    print(f"wrote {OUT_PATH} ({W}x{H})")


if __name__ == "__main__":
    main()
