---
name: sketch-findings-ratimos
description: Validated design decisions, CSS patterns, and visual direction from sketch experiments. Auto-loaded during UI implementation on RatimOS.
---

<context>
## Project: RatimOS

Repaginada visual do RatimOS motivada por feedback direto de UAT da Fase 02.1: o visual
original ("ta horrivel", "todos os botões são ruins de clicar") não estava à altura de um
presente pessoal. Direção validada: estética de handheld retrô com fundo em gradiente
pixelizado dithered (roxo → magenta → laranja), cards com moldura bevel retrô translúcida
sem cantos arredondados, ícones soltos (sem badge circular) com sombra em pixel, e um header
estilo "explorador de arquivos" (`./home/jogos/conexo`) com o peão-torre vermelho como logo
do sistema — reforçando o tema "sistema operacional pessoal".

Sketch sessions wrapped: 2026-09-11
</context>

<design_direction>
## Overall Direction

Paleta de marca (D-17) mantida como base — fundo `#000000` vira o gradiente dithered, painel
`#2a123f` vira translúcido, acento `#e6010f` permanece como cor de destaque em bordas/CTAs.
Tipografia já travada (D-08: Press Start 2P pro Heading/Display, Montserrat pro Body) é
reaproveitada, agora com uma segunda linha de descrição em mono abaixo do título nos cards.
Zero `border-radius` em qualquer superfície de card/painel. Zero emoji na UI — tudo vira
ícone pixel-art (incluindo bateria e o logo do sistema, que passa a ser o peão-torre
vermelho fornecido pela usuária). Sombras em toda a UI são "duras" (offset 2px, sem blur),
nunca `box-shadow` suave de app web.
</design_direction>

<findings_index>
## Design Areas

| Area | Reference | Key Decision |
|------|-----------|--------------|
| Fundo & Ambiente | references/fundo-e-ambiente.md | Gradiente dithered roxo→magenta→laranja + scanlines sutis, como bitmap estático convertido (não gradiente em runtime) |
| Ícones | references/icones.md | Ícone solto sem badge (remove o `lv_obj_create()` clicável fantasma que causava o bug de hitbox), sombra pixel dura |
| Cards & Superfícies | references/cards-superficies.md | Moldura bevel retrô (borda externa 2px + friso interno via inset), fundo translúcido liso, cantos retos |
| Header & Navegação | references/header-navegacao.md | Breadcrumb tipo explorador de arquivos, logo = peão-torre vermelho, bateria em ícone pixel, sem emoji, sem cursor piscando |

## Theme

O CSS de referência (tokens de cor/espaçamento usados nos sketches) está em
`sources/themes/default.css`. **Não é CSS de produção** — é vocabulário visual pra tradução
manual em `src/ratimos/theme.h`/`theme.c` (LVGL/C), não algo a importar.

## Source Files

HTML original dos 4 sketches preservado em `sources/001-background/`, `sources/002-icon-no-badge/`,
`sources/003-card-layers/`, `sources/004-header-breadcrumb/` — cada um com as variantes
descartadas visíveis (não deletadas) e a vencedora marcada com ★ na aba.

Assets processados prontos pra conversão real: `sources/themes/bg-dither.png`,
`bg-dither-scan.png` (fundo), `system_rook.png`/`system_rook_32.png` (logo, já com alpha
real via chroma-key do fundo original enviado pela usuária).
</findings_index>

<metadata>
## Processed Sketches

- 001-background
- 002-icon-no-badge
- 003-card-layers
- 004-header-breadcrumb
</metadata>
