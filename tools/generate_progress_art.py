#!/usr/bin/env python3
"""
Gera os onze PNGs fonte da arte de progressao castelo/jardim
(assets/progress/*.png): seis cenas de estagio (288x180) mais cinco selos de
desbloqueio exclusivo por jogo (32x32) -- de forma procedural (Pillow, sem
editor externo, sem imagem de referencia), mesmo metodo de producao ja usado
pelo icon set (tools/generate_icon_art.py, D-07), agora aplicado a arte de
progressao (D-03/D-06a).

As seis cenas de estagio sao uma UNICA vista continua do mesmo terreno --
cada estagio ACRESCENTA a anterior em vez de substitui-la: terreno vazio ->
alicerce -> muros + primeiro canteiro -> torres + primeiras flores ->
bandeira + jardim cheio -> castelo completo. A partir do estagio 02 toda
cena mostra castelo E jardim no mesmo quadro (D-03's whole point).

Uso: python3 tools/generate_progress_art.py
"""
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "assets" / "progress"

STAGE_W, STAGE_H = 288, 180
UNLOCK_SIZE = 32
GROUND_Y = 150

# Paleta RatimOS (src/ratimos/theme.h, D-17 intacto) mais um pequeno
# conjunto de verdes/terra dedicados exclusivamente ao jardim -- mesmo
# espirito das 7 cores semanticas de jogo em theme.h: nunca usadas como
# cor de chrome/CTA, apenas dentro destas cenas de progressao.
BG          = (0x00, 0x00, 0x00, 255)  # RATIMOS_COLOR_BG
PANEL       = (0x2a, 0x12, 0x3f, 255)  # RATIMOS_COLOR_PANEL
PANEL_ACT   = (0x4e, 0x22, 0x77, 255)  # RATIMOS_COLOR_PANEL_ACTIVE
ACCENT      = (0xe6, 0x01, 0x0f, 255)  # RATIMOS_COLOR_ACCENT
TEXT        = (0xf5, 0xf2, 0xf8, 255)  # RATIMOS_COLOR_TEXT
DIRT        = (0x4a, 0x33, 0x20, 255)
TRUNK       = (0x5a, 0x3d, 0x22, 255)
GREEN_DARK  = (0x1e, 0x5c, 0x33, 255)
GREEN_LIGHT = (0x4c, 0xaf, 0x50, 255)
TRANSPARENT = (0, 0, 0, 0)


def new_stage_canvas():
    return Image.new("RGBA", (STAGE_W, STAGE_H), BG)


def new_unlock_canvas():
    return Image.new("RGBA", (UNLOCK_SIZE, UNLOCK_SIZE), TRANSPARENT)


def draw_ground(d):
    d.rectangle([0, GROUND_Y, STAGE_W, STAGE_H], fill=DIRT)


def draw_plot_outline(d):
    d.rectangle([24, 60, STAGE_W - 24, GROUND_Y], outline=TEXT, width=2)


def draw_walls(d):
    d.rectangle([24, 40, STAGE_W - 24, GROUND_Y], fill=PANEL, outline=TEXT, width=2)
    for x0 in range(30, STAGE_W - 30, 24):
        d.rectangle([x0, 34, x0 + 12, 44], fill=PANEL, outline=TEXT, width=1)


def draw_towers(d):
    for tx in (34, STAGE_W - 34 - 26):
        d.rectangle([tx, 16, tx + 26, GROUND_Y], fill=PANEL_ACT, outline=TEXT, width=2)
        for x0 in range(tx + 2, tx + 26, 8):
            d.rectangle([x0, 10, x0 + 5, 18], fill=PANEL_ACT, outline=TEXT, width=1)


def draw_garden_bed(d, x0, width, bloom_step=14):
    d.rectangle([x0, GROUND_Y - 20, x0 + width, GROUND_Y - 4], fill=DIRT, outline=GREEN_DARK, width=1)
    for cx in range(x0 + 8, x0 + width, bloom_step):
        d.ellipse([cx - 4, GROUND_Y - 28, cx + 4, GROUND_Y - 20], fill=GREEN_LIGHT)
        d.ellipse([cx - 2, GROUND_Y - 30, cx + 2, GROUND_Y - 26], fill=ACCENT)


def draw_stage00(im):
    """Terreno vazio: apenas o contorno do muro baixo sobre a terra nua."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_plot_outline(d)


def draw_stage01(im):
    """Alicerce: pedras de fundacao alinhadas dentro do terreno demarcado."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_plot_outline(d)
    for i in range(6):
        x0 = 30 + i * 38
        d.rectangle([x0, GROUND_Y - 14, x0 + 30, GROUND_Y - 2], fill=PANEL, outline=TEXT, width=1)


def draw_stage02(im):
    """Muros + primeiro canteiro: primeira cena com castelo E jardim (D-03)."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_walls(d)
    draw_garden_bed(d, 70, 60, bloom_step=18)


def draw_stage03(im):
    """Torres florindo: torres levantadas, dois canteiros com as primeiras flores."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_walls(d)
    draw_towers(d)
    draw_garden_bed(d, 70, 50, bloom_step=14)
    draw_garden_bed(d, 150, 50, bloom_step=14)


def draw_stage04(im):
    """Bandeira + jardim cheio: torre central com bandeira, tres canteiros."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_walls(d)
    draw_towers(d)
    d.rectangle([STAGE_W // 2 - 20, 50, STAGE_W // 2 + 20, GROUND_Y], fill=PANEL_ACT, outline=TEXT, width=2)
    pole_x = STAGE_W // 2
    d.line([(pole_x, 20), (pole_x, 50)], fill=TEXT, width=2)
    d.polygon([(pole_x, 20), (pole_x + 18, 27), (pole_x, 34)], fill=ACCENT)
    for bed_x in (40, 110, 180):
        draw_garden_bed(d, bed_x, 50, bloom_step=12)


def draw_stage05(im):
    """Completo: castelo cheio (torres + torre central com ameias) e um
    unico canteiro continuo cobrindo toda a base -- o jardim na sua maior
    extensao."""
    d = ImageDraw.Draw(im)
    draw_ground(d)
    draw_walls(d)
    draw_towers(d)
    d.rectangle([STAGE_W // 2 - 24, 42, STAGE_W // 2 + 24, GROUND_Y], fill=PANEL_ACT, outline=TEXT, width=2)
    for x0 in range(STAGE_W // 2 - 24, STAGE_W // 2 + 24, 12):
        d.rectangle([x0, 36, x0 + 6, 44], fill=PANEL_ACT, outline=TEXT, width=1)
    pole_x = STAGE_W // 2
    d.line([(pole_x, 14), (pole_x, 42)], fill=TEXT, width=2)
    d.polygon([(pole_x, 14), (pole_x + 20, 21), (pole_x, 28)], fill=ACCENT)
    draw_garden_bed(d, 30, STAGE_W - 60, bloom_step=12)


def draw_unlock_sudoku_roseira(im):
    """Roseira: arbusto verde com flores vermelhas (Sudoku)."""
    d = ImageDraw.Draw(im)
    d.ellipse([6, 14, 26, 30], fill=GREEN_DARK, outline=GREEN_LIGHT, width=1)
    for cx, cy in [(11, 18), (16, 15), (21, 19), (14, 23), (20, 24)]:
        d.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=ACCENT)


def draw_unlock_paciencia_bandeira(im):
    """Bandeirola numa haste (Paciencia)."""
    d = ImageDraw.Draw(im)
    d.line([(10, 6), (10, 27)], fill=TEXT, width=2)
    d.polygon([(10, 6), (24, 11), (10, 16)], fill=ACCENT)
    d.rectangle([6, 26, 14, 29], fill=PANEL_ACT)


def draw_unlock_termo_arvore(im):
    """Arvore pequena (Termo)."""
    d = ImageDraw.Draw(im)
    d.rectangle([14, 20, 18, 28], fill=TRUNK)
    d.ellipse([6, 5, 26, 23], fill=GREEN_DARK, outline=GREEN_LIGHT, width=1)


def draw_unlock_cruzadinha_fonte(im):
    """Fonte de jardim (Cruzadinha)."""
    d = ImageDraw.Draw(im)
    d.ellipse([6, 20, 26, 28], fill=PANEL_ACT, outline=TEXT, width=1)
    d.rectangle([14, 10, 18, 22], fill=PANEL_ACT, outline=TEXT, width=1)
    d.ellipse([12, 4, 20, 12], fill=PANEL_ACT, outline=TEXT, width=1)
    for cx in (11, 16, 21):
        d.line([(cx, 13), (cx, 18)], fill=TEXT, width=1)


def draw_unlock_conexo_portao(im):
    """Portao em arco (Conexo)."""
    d = ImageDraw.Draw(im)
    d.rectangle([8, 16, 24, 27], outline=TEXT, width=2)
    d.arc([8, 4, 24, 24], 180, 360, fill=TEXT, width=2)
    for x in (11, 16, 21):
        d.line([(x, 16), (x, 27)], fill=TEXT, width=1)


STAGES = {
    "castle_stage_00_terreno_vazio": (new_stage_canvas, draw_stage00),
    "castle_stage_01_alicerce": (new_stage_canvas, draw_stage01),
    "castle_stage_02_muros_canteiro": (new_stage_canvas, draw_stage02),
    "castle_stage_03_torres_florindo": (new_stage_canvas, draw_stage03),
    "castle_stage_04_bandeira_jardim_cheio": (new_stage_canvas, draw_stage04),
    "castle_stage_05_completo": (new_stage_canvas, draw_stage05),
}

UNLOCKS = {
    "unlock_sudoku_roseira": (new_unlock_canvas, draw_unlock_sudoku_roseira),
    "unlock_paciencia_bandeira": (new_unlock_canvas, draw_unlock_paciencia_bandeira),
    "unlock_termo_arvore": (new_unlock_canvas, draw_unlock_termo_arvore),
    "unlock_cruzadinha_fonte": (new_unlock_canvas, draw_unlock_cruzadinha_fonte),
    "unlock_conexo_portao": (new_unlock_canvas, draw_unlock_conexo_portao),
}


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for asset_id, (new_canvas, draw_fn) in {**STAGES, **UNLOCKS}.items():
        im = new_canvas()
        draw_fn(im)
        out_path = OUT_DIR / f"{asset_id}.png"
        im.save(out_path)
        print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
