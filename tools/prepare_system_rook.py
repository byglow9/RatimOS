#!/usr/bin/env python3
"""
Pre-processa a arte do peao-torre vermelho (referencia fornecida pela
usuaria, ja chroma-key'd nos sketches em
.claude/skills/sketch-findings-ratimos/sources/themes/system_rook.png) para
um PNG fonte compativel com o pipeline real de icones
(tools/convert_images.py):

  1. Threshold do canal alpha pra binario (0 ou 255) -- remove a borda
     anti-aliased suave do chroma-key, que produz ~500 cores distintas
     incompativeis com o teto de 16 cores do formato LV_COLOR_FORMAT_I4
     usado por todo o resto do icon set.
  2. Quantizacao dos pixels opacos pra uma paleta pequena (poucas cores),
     bem abaixo do teto de 16.
  3. Downscale com Image.NEAREST (nunca bicubico/bilinear, que reintroduz
     cores intermediarias e quebra o orcamento de paleta) pro tamanho final
     usado no topbar (altura da linha do topbar: 26px).

Isso e' preparacao de asset (posterizar/quantizar uma imagem de referencia
externa), nao geracao de arte nova -- por isso um script avulso em vez de um
`generate_*.py` dedicado. Mantido no repo (nao descartado) pra o passo ser
reproduzivel.

Uso:
  python3 tools/prepare_system_rook.py
"""
from pathlib import Path

from PIL import Image

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCE = REPO_ROOT / ".claude/skills/sketch-findings-ratimos/sources/themes/system_rook.png"
OUT = REPO_ROOT / "assets/icons/system_rook.png"

ALPHA_CUTOFF = 128
PALETTE_COLORS = 6
TARGET_HEIGHT = 20


def main() -> int:
    im = Image.open(SOURCE).convert("RGBA")
    w, h = im.size
    px = im.load()

    # 1) Threshold alpha para bordas duras (remove o anti-aliasing do
    #    chroma-key da imagem de referencia).
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            px[x, y] = (0, 0, 0, 0) if a < ALPHA_CUTOFF else (r, g, b, 255)

    # 2) Quantiza somente os pixels opacos -- os pixels transparentes nunca
    #    entram no orcamento de paleta do quantizador.
    rgb = im.convert("RGB")
    quantized_rgb = rgb.quantize(colors=PALETTE_COLORS, method=Image.MEDIANCUT).convert("RGB")
    quantized = Image.new("RGBA", im.size, (0, 0, 0, 0))
    qpx = quantized.load()
    qrgbpx = quantized_rgb.load()
    for y in range(h):
        for x in range(w):
            _, _, _, a = px[x, y]
            if a == 255:
                r, g, b = qrgbpx[x, y]
                qpx[x, y] = (r, g, b, 255)

    # 3) Resize NEAREST pro tamanho final -- preserva exatamente a paleta
    #    quantizada (nenhuma cor nova por interpolacao).
    target_h = TARGET_HEIGHT
    target_w = round(w * target_h / h)
    resized = quantized.resize((target_w, target_h), Image.NEAREST)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    resized.save(OUT)

    colors = resized.getcolors(maxcolors=100000)
    n_colors = len(colors) if colors else -1
    print(f"wrote {OUT} ({target_w}x{target_h}, {n_colors} distinct RGBA colors)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
