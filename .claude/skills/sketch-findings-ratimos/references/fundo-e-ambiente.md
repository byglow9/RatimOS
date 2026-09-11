# Fundo & Ambiente

## Design Decisions

O fundo flat quase-preto (`RATIMOS_COLOR_BG = 0x000000`) foi rejeitado em UAT ("o background
deve ser mais assim" + imagem de referência). Vencedor: **gradiente pixelizado dithered**
(sketch 001, variante C) — roxo escuro no topo → magenta/rosa no meio → laranja quente
embaixo, com dithering ordenado (Bayer 4x4) sobre uma paleta de ~10 níveis por canal, mais
scanlines horizontais sutis (1px a cada 3px, ~12% opacidade).

Por que C venceu sobre A (gradiente liso) e B (dithered sem scanline): a usuária achou o
resultado "muito perfeito" com as scanlines — reforça o clima de handheld/console retrô que
o resto da identidade (Press Start 2P, pixel-art) já estabelece. Um gradiente liso (`lv_style_set_bg_grad`)
seria mais barato de implementar mas lê como "app moderno", não retrô.

## Paleta dos 4 stops (vertical, topo → base)

```
#1c0a3d  (0%)
#7a1560  (~42% do gradiente, 3-stop lerp)
#c81f4c  (~68%)
#e8630f  (100%)
```

## Implementação de referência (browser, Canvas 2D — ver `sources/001-background/index.html`)

O dithering foi gerado em baixa resolução (80x120) com uma matriz Bayer 4x4 e depois ampliado
com `image-rendering: pixelated` / nearest-neighbor. Script Python equivalente (Pillow) em
`sources/themes/` (via `gen_dither_bg.py` no `.planning/sketches/themes/`, não copiado pra cá
pois é gerador, não asset final) produziu os PNGs estáticos `bg-dither.png` e `bg-dither-scan.png`
(320x480) usados nos sketches subsequentes.

## Path pra implementação real em LVGL/ESP32-S3

**Não é um `lv_style_set_bg_grad` em runtime.** É um bitmap estático, gerado uma vez no
mesmo pipeline Pillow já usado pros ícones (D-07, ver `docs/visual-identity/README.md` do
projeto) e convertido pra array C `LV_COLOR_FORMAT_*` via `lv_img_conv`/o script existente.
Motivo: dithering ordenado pixel-a-pixel não é algo que valha a pena recalcular em runtime
num MCU, e um PNG 320x480 indexado (paleta pequena, poucos tons distintos por causa do
dithering) comprime bem — orçamento de flash deve ficar abaixo de ~15-20KB, mas medir de
verdade antes de travar (RAM/flash budget não verificado nesta sessão, é só HTML).

Sugestão de tarefa pro plano de execução: gerar `bg_dither.png` → `ratimos_bg_dither.c`
(array C, mesmo padrão dos ícones), aplicar como `lv_img` de fundo por trás de `ratimos_theme_apply_screen()`
(ou como estilo `bg_img_src` no objeto de tela raiz), substituindo `RATIMOS_COLOR_BG` sólido.

## What to Avoid

- Gradiente liso via `lv_style_set_bg_grad` (variante A) — foi preterido, não é o que a
  usuária escolheu; não usar como atalho "mais fácil".
- Scanlines fortes/opacidade alta — a versão validada é sutil (~12%); mais que isso arrisca
  "sujar" texto pequeno (risco já sinalizado no README do sketch, não testado a fundo).

## Origin

Synthesized from sketch: 001 (winner: C)
Source files available in: `sources/001-background/index.html`, `sources/themes/bg-dither-scan.png`
