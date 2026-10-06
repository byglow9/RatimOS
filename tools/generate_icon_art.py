#!/usr/bin/env python3
"""
Gera as PNGs de pixel art fonte em assets/icons/*.png de forma
procedural (nao traçada de nenhuma imagem de referencia) -- e' o metodo de
producao escolhido para o icon set desta fase (D-07): em vez de um editor
externo (Piskel/Aseprite, a opcao levantada no UI-SPEC), um script
project-owned desenha cada icone com formas geometricas simples via
Pillow, usando SOMENTE as cores do tema RatimOS (RATIMOS_COLOR_TEXT e
RATIMOS_COLOR_PANEL_ACTIVE, mais fundo transparente), garantindo que a
arte seja 100% autoral, deterministica e reproduzivel a partir do codigo
fonte -- sem depender de nenhum arquivo binario nao versionado.

Tamanhos (plano 02.1-14): os icones de launcher da home (home_*) sao
32x32; os icones de LINHA de lista (game_*, row_*, cfg_*) sao 26x26 -- o
tamanho exato do `.row-inner img` do sketch 003-C. Cada desenho e' escrito
num espaco de coordenadas 32x32 e rasterizado DIRETO no tamanho final por
ScaledDraw (coordenadas mapeadas antes de desenhar, linhas >= 1px): nada e'
reamostrado depois, entao o icone de 26px continua nitido, sem o borrado
de um downscale e sem o zoom fracionario no LVGL.

Uso: python3 tools/generate_icon_art.py
"""
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "assets" / "icons"

SIZE = 32       # espaco de coordenadas de autoria de todo desenho
ROW_SIZE = 26   # tamanho final dos icones de linha de lista (sketch 003-C)

# Mesma paleta de RATIMOS_COLOR_TEXT / RATIMOS_COLOR_PANEL_ACTIVE
# (src/ratimos/theme.h) -- os icones usam apenas estas duas cores mais
# fundo transparente, para lerem como uma familia visual consistente e
# para nunca colidirem com o preenchimento vermelho (RATIMOS_COLOR_ACCENT)
# do selo/badge circular por tras de cada icone.
TEXT = (0xF5, 0xF2, 0xF8, 255)
PANEL_ACTIVE = (0x4E, 0x22, 0x77, 255)
TRANSPARENT = (0, 0, 0, 0)


def new_canvas(size=SIZE):
    return Image.new("RGBA", (size, size), TRANSPARENT)


class ScaledDraw:
    """ImageDraw que recebe coordenadas no espaco de autoria 32x32 e
    desenha direto na resolucao real do canvas (ex. 26x26) -- mesma API
    dos metodos de ImageDraw usados abaixo. Escala 1.0 (canvas 32x32)
    produz exatamente o mesmo desenho de antes."""

    def __init__(self, im):
        self._d = ImageDraw.Draw(im)
        self._k = im.size[0] / SIZE

    def _v(self, v):
        return int(round(v * self._k))

    def _pt(self, pt):
        return (self._v(pt[0]), self._v(pt[1]))

    def _box(self, box):
        x0, y0, x1, y1 = box
        return [self._v(x0), self._v(y0), self._v(x1), self._v(y1)]

    def _w(self, width):
        return max(1, int(round(width * self._k)))

    def rectangle(self, box, fill=None, outline=None, width=1):
        self._d.rectangle(self._box(box), fill=fill, outline=outline, width=self._w(width))

    def rounded_rectangle(self, box, radius=0, fill=None, outline=None, width=1):
        self._d.rounded_rectangle(self._box(box), radius=max(0, self._v(radius)), fill=fill,
                                  outline=outline, width=self._w(width))

    def ellipse(self, box, fill=None, outline=None, width=1):
        self._d.ellipse(self._box(box), fill=fill, outline=outline, width=self._w(width))

    def polygon(self, pts, fill=None, outline=None):
        self._d.polygon([self._pt(p) for p in pts], fill=fill, outline=outline)

    def line(self, pts, fill=None, width=1):
        self._d.line([self._pt(p) for p in pts], fill=fill, width=self._w(width))


def draw_home_jogos(im):
    """Dado de 5 (controle/jogo generico)."""
    d = ScaledDraw(im)
    d.rounded_rectangle([4, 4, 27, 27], radius=5, fill=PANEL_ACTIVE, outline=TEXT, width=2)
    for cx, cy in [(10, 10), (21, 10), (10, 21), (21, 21), (15, 15)]:
        d.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=TEXT)


def draw_home_musica(im):
    """Nota musical."""
    d = ScaledDraw(im)
    d.ellipse([6, 20, 16, 28], fill=TEXT)
    d.rectangle([14, 6, 17, 24], fill=TEXT)
    d.polygon([(17, 6), (26, 9), (26, 16), (17, 13)], fill=TEXT)


def draw_home_album(im):
    """Moldura de foto com paisagem simples."""
    d = ScaledDraw(im)
    d.rounded_rectangle([4, 6, 27, 25], radius=2, outline=TEXT, width=2)
    d.polygon([(8, 22), (14, 14), (19, 22)], fill=PANEL_ACTIVE)
    d.ellipse([20, 9, 25, 14], fill=TEXT)


def draw_home_cartas(im):
    """Envelope."""
    d = ScaledDraw(im)
    d.rectangle([4, 8, 27, 24], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.line([(4, 8), (15, 17), (27, 8)], fill=TEXT, width=2)


def draw_home_config(im):
    """Engrenagem."""
    d = ScaledDraw(im)
    for cx, cy in [(15, 5), (15, 26), (5, 15), (26, 15)]:
        d.rectangle([cx - 2, cy - 2, cx + 2, cy + 2], fill=TEXT)
    d.ellipse([8, 8, 23, 23], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.ellipse([13, 13, 18, 18], fill=TRANSPARENT, outline=TEXT, width=1)


def draw_home_castelo(im):
    """Torre de castelo (reusa o motivo da torre da logo, simplificado)."""
    d = ScaledDraw(im)
    d.rectangle([10, 14, 21, 27], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    for x0 in (9, 15, 21):
        d.rectangle([x0, 8, x0 + 3, 14], fill=PANEL_ACTIVE, outline=TEXT, width=1)
    d.rectangle([14, 19, 17, 24], fill=TEXT)


def draw_game_sudoku(im):
    """Grade 3x3 com algumas celulas preenchidas."""
    d = ScaledDraw(im)
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
    d = ScaledDraw(im)
    d.rounded_rectangle([8, 4, 23, 27], radius=2, fill=TEXT, outline=PANEL_ACTIVE, width=2)
    d.polygon([(11, 8), (13, 11), (11, 14), (9, 11)], fill=PANEL_ACTIVE)


def draw_game_termo(im):
    """Peça-letra (bloco com uma glifo 'T' estilizada)."""
    d = ScaledDraw(im)
    d.rounded_rectangle([5, 5, 26, 26], radius=3, fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.rectangle([10, 11, 21, 14], fill=TEXT)
    d.rectangle([14, 14, 17, 23], fill=TEXT)


def draw_game_cruzadinha(im):
    """Mini grade de palavras cruzadas com celulas bloqueadas."""
    d = ScaledDraw(im)
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
    d = ScaledDraw(im)
    pts = {"tl": (11, 11), "tr": (21, 11), "bl": (11, 21), "br": (21, 21)}
    d.line([pts["tl"], pts["tr"]], fill=PANEL_ACTIVE, width=2)
    d.line([pts["bl"], pts["br"]], fill=PANEL_ACTIVE, width=2)
    for cx, cy in pts.values():
        d.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=TEXT)


# ---- Icones de linha novos (plano 02.1-14): substituem as letras cruas
# ("!", "B", "V", "F", "S", "L", "P", "T") que ratimos_badge_create()
# mostrava por falta de icone compilado. Mesmas 2 cores, mesmo espaco 32.

def draw_row_empty(im):
    """Bandeja vazia (estado vazio de qualquer lista)."""
    d = ScaledDraw(im)
    d.line([(4, 14), (4, 26), (27, 26), (27, 14)], fill=TEXT, width=2)
    d.rectangle([9, 18, 22, 21], fill=PANEL_ACTIVE)
    d.line([(9, 7), (12, 10)], fill=TEXT, width=2)
    d.line([(22, 7), (19, 10)], fill=TEXT, width=2)
    d.line([(15, 5), (15, 10)], fill=TEXT, width=2)


def draw_row_carta(im):
    """Envelope fechado com selo quadrado no bico da aba."""
    d = ScaledDraw(im)
    d.rectangle([3, 7, 28, 25], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.line([(3, 7), (15, 16), (28, 7)], fill=TEXT, width=2)
    d.rectangle([13, 16, 18, 21], fill=TEXT)


def draw_row_playlist(im):
    """Lista de faixas (3 linhas) com uma nota ao lado."""
    d = ScaledDraw(im)
    for y in (7, 13, 19):
        d.rectangle([3, y, 16, y + 2], fill=TEXT)
    d.ellipse([16, 21, 23, 27], fill=TEXT)
    d.rectangle([21, 6, 23, 24], fill=TEXT)
    d.polygon([(23, 6), (29, 9), (29, 13), (23, 10)], fill=PANEL_ACTIVE)


def draw_row_track(im):
    """Botao de tocar (triangulo) dentro de um quadro."""
    d = ScaledDraw(im)
    d.rectangle([4, 4, 27, 27], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.polygon([(12, 9), (22, 15), (12, 22)], fill=TEXT)


def draw_cfg_brilho(im):
    """Sol: disco + 8 raios."""
    d = ScaledDraw(im)
    d.ellipse([10, 10, 21, 21], fill=TEXT)
    for (x0, y0, x1, y1) in [(15, 2, 15, 6), (15, 25, 15, 29), (2, 15, 6, 15), (25, 15, 29, 15),
                             (6, 6, 8, 8), (23, 23, 25, 25), (6, 25, 8, 23), (23, 8, 25, 6)]:
        d.line([(x0, y0), (x1, y1)], fill=TEXT, width=2)


def draw_cfg_volume(im):
    """Alto-falante com duas ondas."""
    d = ScaledDraw(im)
    d.rectangle([3, 12, 8, 19], fill=TEXT)
    d.polygon([(8, 12), (15, 6), (15, 25), (8, 19)], fill=TEXT)
    d.line([(19, 12), (21, 15), (19, 19)], fill=PANEL_ACTIVE, width=2)
    d.line([(23, 8), (27, 15), (23, 23)], fill=TEXT, width=2)


def draw_cfg_firmware(im):
    """Chip: quadrado com pinos nos 4 lados."""
    d = ScaledDraw(im)
    d.rectangle([8, 8, 23, 23], fill=PANEL_ACTIVE, outline=TEXT, width=2)
    d.rectangle([13, 13, 18, 18], fill=TEXT)
    for t in (11, 15, 19):
        d.line([(t, 3), (t, 7)], fill=TEXT, width=2)
        d.line([(t, 24), (t, 28)], fill=TEXT, width=2)
        d.line([(3, t), (7, t)], fill=TEXT, width=2)
        d.line([(24, t), (28, t)], fill=TEXT, width=2)


def draw_cfg_armazenamento(im):
    """Cartao microSD: canto chanfrado + contatos."""
    d = ScaledDraw(im)
    d.polygon([(8, 3), (25, 3), (25, 28), (6, 28), (6, 9), (8, 7)], fill=TEXT)
    d.polygon([(10, 6), (22, 6), (22, 25), (9, 25), (9, 10)], fill=PANEL_ACTIVE)
    for x in (11, 15, 19):
        d.rectangle([x, 7, x + 2, 12], fill=TEXT)


# id -> (funcao de desenho, tamanho final em px). home_* = launcher 32px;
# game_*/row_*/cfg_* = linha de lista 26px (sketch 003-C).
ICONS = {
    "home_jogos": (draw_home_jogos, SIZE),
    "home_musica": (draw_home_musica, SIZE),
    "home_album": (draw_home_album, SIZE),
    "home_cartas": (draw_home_cartas, SIZE),
    "home_config": (draw_home_config, SIZE),
    "home_castelo": (draw_home_castelo, SIZE),
    "game_sudoku": (draw_game_sudoku, ROW_SIZE),
    "game_paciencia": (draw_game_paciencia, ROW_SIZE),
    "game_termo": (draw_game_termo, ROW_SIZE),
    "game_cruzadinha": (draw_game_cruzadinha, ROW_SIZE),
    "game_conexo": (draw_game_conexo, ROW_SIZE),
    "row_empty": (draw_row_empty, ROW_SIZE),
    "row_carta": (draw_row_carta, ROW_SIZE),
    "row_playlist": (draw_row_playlist, ROW_SIZE),
    "row_track": (draw_row_track, ROW_SIZE),
    "cfg_brilho": (draw_cfg_brilho, ROW_SIZE),
    "cfg_volume": (draw_cfg_volume, ROW_SIZE),
    "cfg_firmware": (draw_cfg_firmware, ROW_SIZE),
    "cfg_armazenamento": (draw_cfg_armazenamento, ROW_SIZE),
}


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for icon_id, (draw_fn, size) in ICONS.items():
        im = new_canvas(size)
        draw_fn(im)
        out_path = OUT_DIR / f"{icon_id}.png"
        im.save(out_path)
        print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
