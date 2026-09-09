#!/usr/bin/env python3
"""
Gera as onze PNGs de pixel art fonte em assets/icons/*.png de forma
procedural (nao traçada de nenhuma imagem de referencia) -- e' o metodo de
producao escolhido para o icon set desta fase (D-07): em vez de um editor
externo (Piskel/Aseprite, a opcao levantada no UI-SPEC), um script
project-owned desenha cada icone com formas geometricas simples via
Pillow, usando SOMENTE as cores do tema RatimOS (RATIMOS_COLOR_TEXT e
RATIMOS_COLOR_PANEL_ACTIVE, mais fundo transparente), garantindo que a
arte seja 100% autoral, deterministica e reproduzivel a partir do codigo
fonte -- sem depender de nenhum arquivo binario nao versionado.

Uso: python3 tools/generate_icon_art.py
"""
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "assets" / "icons"

SIZE = 32

# Mesma paleta de RATIMOS_COLOR_TEXT / RATIMOS_COLOR_PANEL_ACTIVE
# (src/ratimos/theme.h) -- os icones usam apenas estas duas cores mais
# fundo transparente, para lerem como uma familia visual consistente e
# para nunca colidirem com o preenchimento vermelho (RATIMOS_COLOR_ACCENT)
# do selo/badge circular por tras de cada icone.
TEXT = (0xF5, 0xF2, 0xF8, 255)
PANEL_ACTIVE = (0x4E, 0x22, 0x77, 255)
TRANSPARENT = (0, 0, 0, 0)


def new_canvas():
    return Image.new("RGBA", (SIZE, SIZE), TRANSPARENT)


def draw_home_jogos(im):
    """Dado de 5 (controle/jogo generico)."""
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([4, 4, 27, 27], radius=5, fill=PANEL_ACTIVE, outline=TEXT, width=2)
    for cx, cy in [(10, 10), (21, 10), (10, 21), (21, 21), (15, 15)]:
        d.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=TEXT)


def draw_home_musica(im):
    """Nota musical."""
    d = ImageDraw.Draw(im)
    d.ellipse([6, 20, 16, 28], fill=TEXT)
    d.rectangle([14, 6, 17, 24], fill=TEXT)
    d.polygon([(17, 6), (26, 9), (26, 16), (17, 13)], fill=TEXT)


def draw_home_album(im):
    """Moldura de foto com paisagem simples."""
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([4, 6, 27, 25], radius=2, outline=TEXT, width=2)
    d.polygon([(8, 22), (14, 14), (19, 22)], fill=PANEL_ACTIVE)
    d.ellipse([20, 9, 25, 14], fill=TEXT)


def draw_home_cartas(im):
    """Envelope."""
    d = ImageDraw.Draw(im)
    d.rectangle([4, 8, 27, 24], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.line([(4, 8), (15, 17), (27, 8)], fill=TEXT, width=2)


def draw_home_config(im):
    """Engrenagem."""
    d = ImageDraw.Draw(im)
    for cx, cy in [(15, 5), (15, 26), (5, 15), (26, 15)]:
        d.rectangle([cx - 2, cy - 2, cx + 2, cy + 2], fill=TEXT)
    d.ellipse([8, 8, 23, 23], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.ellipse([13, 13, 18, 18], fill=TRANSPARENT, outline=TEXT, width=1)


def draw_home_castelo(im):
    """Torre de castelo (reusa o motivo da torre da logo, simplificado)."""
    d = ImageDraw.Draw(im)
    d.rectangle([10, 14, 21, 27], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    for x0 in (9, 15, 21):
        d.rectangle([x0, 8, x0 + 3, 14], fill=PANEL_ACTIVE, outline=TEXT, width=1)
    d.rectangle([14, 19, 17, 24], fill=TEXT)


def draw_game_sudoku(im):
    """Grade 3x3 com algumas celulas preenchidas."""
    d = ImageDraw.Draw(im)
    d.rectangle([6, 6, 25, 25], outline=TEXT, width=2)
    for i in (1, 2):
        x = 6 + i * (19 // 3)
        d.line([(x, 6), (x, 25)], fill=TEXT, width=1)
        y = 6 + i * (19 // 3)
        d.line([(6, y), (25, y)], fill=TEXT, width=1)
    for cx, cy in [(10, 10), (15, 15), (21, 21)]:
        d.rectangle([cx - 1, cy - 1, cx + 1, cy + 1], fill=PANEL_ACTIVE)


def draw_game_paciencia(im):
    """Carta de baralho com naipe."""
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([8, 4, 23, 27], radius=2, fill=TEXT, outline=PANEL_ACTIVE, width=2)
    d.polygon([(11, 8), (13, 11), (11, 14), (9, 11)], fill=PANEL_ACTIVE)


def draw_game_termo(im):
    """Peça-letra (bloco com uma glifo 'T' estilizada)."""
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([5, 5, 26, 26], radius=3, fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.rectangle([10, 11, 21, 14], fill=TEXT)
    d.rectangle([14, 14, 17, 23], fill=TEXT)


def draw_game_cruzadinha(im):
    """Mini grade de palavras cruzadas com celulas bloqueadas."""
    d = ImageDraw.Draw(im)
    cell = 5
    x0, y0 = 6, 6
    for row in range(4):
        for col in range(4):
            x = x0 + col * cell
            y = y0 + row * cell
            blocked = (row, col) in {(0, 3), (2, 1), (3, 3)}
            d.rectangle([x, y, x + cell, y + cell],
                        fill=PANEL_ACTIVE if blocked else TRANSPARENT,
                        outline=TEXT, width=1)


def draw_game_conexo(im):
    """Quatro pontos agrupados em pares (grupos conectados)."""
    d = ImageDraw.Draw(im)
    pts = {"tl": (11, 11), "tr": (21, 11), "bl": (11, 21), "br": (21, 21)}
    d.line([pts["tl"], pts["tr"]], fill=PANEL_ACTIVE, width=2)
    d.line([pts["bl"], pts["br"]], fill=PANEL_ACTIVE, width=2)
    for cx, cy in pts.values():
        d.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=TEXT)


ICONS = {
    "home_jogos": draw_home_jogos,
    "home_musica": draw_home_musica,
    "home_album": draw_home_album,
    "home_cartas": draw_home_cartas,
    "home_config": draw_home_config,
    "home_castelo": draw_home_castelo,
    "game_sudoku": draw_game_sudoku,
    "game_paciencia": draw_game_paciencia,
    "game_termo": draw_game_termo,
    "game_cruzadinha": draw_game_cruzadinha,
    "game_conexo": draw_game_conexo,
}


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for icon_id, draw_fn in ICONS.items():
        im = new_canvas()
        draw_fn(im)
        out_path = OUT_DIR / f"{icon_id}.png"
        im.save(out_path)
        print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
