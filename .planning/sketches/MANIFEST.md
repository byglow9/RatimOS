# Sketch Manifest

## Design Direction

Repaginada visual do RatimOS motivada por feedback direto de UAT (Fase 02.1): o visual atual
("ta horrivel", "todos os botões são ruins de clicar") não está à altura de um presente
pessoal. Direção: estética de handheld retrô com fundo em gradiente pixelizado
dithered (roxo → magenta → laranja, referência fornecida pela usuária), cards com textura
quadriculada e sensação de profundidade em camadas (tipo baralho de cartas), ícones soltos
(sem badge circular vermelho), e um header estilo "explorador de arquivos"
(`./home/jogos/conexo`) com relógio — reforçando o tema "sistema operacional pessoal".

## Reference Points

- Imagem de referência do gradiente enviada pela usuária (`.planning/sketches/001-background/reference-gradient.png`)
- Paleta de marca já travada (D-17, `src/ratimos/theme.h`): fundo preto, painel roxo `#2a123f`, acento vermelho `#e6010f`
- Fonte pixel Press Start 2P (D-08, já decidida pro Heading/Display)
- Ícones pixel-art reais do projeto (`assets/icons/*.png`, D-07)

## Root cause encontrada (não é visual — fix direto de código)

`ratimos_badge_create()` (src/ratimos/theme.c:36) usa `lv_obj_create()` pro badge circular,
que no LVGL vem clicável por padrão. Sem handler próprio, o badge intercepta o toque no
centro do ícone (onde o olho mais mira) e não repassa pro `row` que tem o `click_cb` real —
essa é a causa provável de "tive que clicar umas 10 vezes" (G-02.1-1). Fix: `lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE)`
(ou remover o badge de vez, ver sketch 002).

## Sketches

| # | Name | Design Question | Winner | Tags |
|---|------|----------------|--------|------|
| 001 | background | O gradiente dithered roxo→laranja funciona atrás dos cards? | **C** — dithered + scanline | background, gradient |
| 002 | icon-no-badge | Ícone solto (sem badge vermelho) fica legível na lista e na home? | **B** — solto + sombra pixel (sem blur) | icon |
| 003 | card-layers (v2) | Card sem cantos arredondados, levemente transparente — qual das 5? | **C** — moldura bevel retrô | card, typography |
| 004 | header-breadcrumb | Header tipo explorador (./home/jogos/conexo) + relógio, fonte melhor | **A** — refinado: logo peão-torre, bateria pixel, sem emoji, sem piscar | header, navigation |

## Locked — direção final (2026-09-11)

- **Fundo:** gradiente dithered roxo→magenta→laranja + scanlines sutis (001-C)
- **Ícones:** soltos, sem badge/container, sombra pixel dura (2px offset, sem blur) — `.pixel-shadow` em `themes/default.css`
- **Logo do sistema:** peão-torre vermelho (referência da usuária, `assets/icons/system_rook_32.png`), nenhum emoji na UI
- **Header:** breadcrumb tipo explorador de arquivos (`./home/jogos/conexo`), sem cursor piscando; bateria indicada por ícone pixel-art bloco, não emoji
- **Card:** moldura dupla estilo bevel retrô (GameBoy/Win95) — borda externa escura 2px + friso interno claro 1px via inset, fundo roxo translúcido liso (sem textura/quadriculado), cantos 100% retos (003-C)

Todos os 4 esboços têm vencedor definido — kit visual completo pra fase 02.1.
