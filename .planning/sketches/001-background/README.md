---
sketch: 001
name: background
design_question: O gradiente pixelizado dithered (roxo→magenta→laranja) funciona como fundo de tela atrás dos cards existentes?
winner: "C"
tags: [background, gradient, dithering]
---

## Variants

- **A — Gradiente liso:** `lv_style_set_bg_grad` de 4 stops, sem custo de asset. Mais "app moderno", menos retrô.
- **B — Gradiente dithered pixel-art:** dithering ordenado (Bayer 4x4) sobre os mesmos 4 stops, renderizado em baixa resolução e ampliado com pixel nítido — reproduz o efeito da imagem de referência enviada pela usuária. Buildável como bitmap C convertido (mesmo pipeline Pillow do D-07).
- **C — B + scanlines:** reforça o clima de handheld antigo; risco de "sujar" texto pequeno, testar legibilidade antes de travar.

## Notes

Assets gerados por `.planning/sketches/themes/gen_dither_bg.py` (script descartável, não faz parte do firmware).
