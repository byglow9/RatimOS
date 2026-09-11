---
sketch: 002
name: icon-no-badge
design_question: O ícone pixel-art solto (sem o badge circular vermelho) fica legível e bem alinhado nas linhas de lista e nos tiles da home?
winner: "B"
tags: [icon]
---

## Variants

- **A — Ícone solto:** sem nenhum container atrás, ícone real do projeto (`assets/icons/*.png`).
- **B — Ícone solto + sombra sutil:** `drop-shadow` leve pra separar do fundo colorido/texturizado.
- **C — Antes/depois:** comparação lado a lado com o badge circular vermelho atual.

## Root cause relacionada (não é design, é bug de código)

`ratimos_badge_create()` cria o badge via `lv_obj_create()`, que vem clicável por padrão no LVGL —
ele intercepta o toque no centro do ícone sem repassar pro `row` (que tem o handler real). Isso
provavelmente é a causa raiz de G-02.1-1 ("tive que clicar umas 10 vezes"). Removendo o badge
(variantes A/B) ou pelo menos limpando `LV_OBJ_FLAG_CLICKABLE` nele resolve os dois problemas
(visual + hitbox) de uma vez.
