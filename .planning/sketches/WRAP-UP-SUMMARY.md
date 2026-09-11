# Sketch Wrap-Up Summary

**Date:** 2026-09-11
**Sketches processed:** 4
**Design areas:** Fundo & Ambiente, Ícones, Cards & Superfícies, Header & Navegação
**Skill output:** `./.claude/skills/sketch-findings-ratimos/`

## Included Sketches
| # | Name | Winner | Design Area |
|---|------|--------|-------------|
| 001 | background | C — dithered + scanlines | Fundo & Ambiente |
| 002 | icon-no-badge | B — solto + sombra pixel (sem blur) | Ícones |
| 003 | card-layers (v2) | C — moldura bevel retrô | Cards & Superfícies |
| 004 | header-breadcrumb | A — refinado (logo peão-torre, bateria pixel, sem emoji/piscar) | Header & Navegação |

## Excluded Sketches

Nenhum — todos os 4 incluídos integralmente.

## Design Direction

Handheld retrô: gradiente dithered roxo→magenta→laranja no fundo, cards translúcidos com
moldura bevel (cantos retos), ícones soltos com sombra pixel dura, header em formato de
caminho de explorador de arquivos com o peão-torre vermelho como logo do sistema. Zero
emoji, zero border-radius em superfícies de card.

## Key Decisions

- **Palette:** paleta de marca D-17 mantida, agora com opacidade/translucidez calibrada
  contra o novo fundo em gradiente.
- **Typography:** D-08 (Press Start 2P Heading/Display + Montserrat Body) reaproveitada,
  cards ganham uma segunda linha de descrição em mono.
- **Spacing:** cantos retos em todo card/painel — reversão explícita do `radius: 6px`
  atual de `ratimos_panel_create()`.
- **Interaction:** achado colateral — o badge circular clicável por padrão
  (`lv_obj_create()` sem `LV_OBJ_FLAG_CLICKABLE` limpo) era a causa provável do bug de
  hitbox reportado em UAT (G-02.1-1); removê-lo resolve visual + funcional junto.

## Root Cause Note (carry into plan)

`ratimos_badge_create()` (`src/ratimos/theme.c:36`) usa `lv_obj_create()`, clicável por
padrão no LVGL, sem handler próprio — intercepta o toque no ícone sem repassar pro `row`
pai. Ver `references/icones.md` na skill gerada para o fix completo.
