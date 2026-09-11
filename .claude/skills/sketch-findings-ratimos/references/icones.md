# Ícones

## Design Decisions

O badge circular vermelho atrás dos ícones (`ratimos_badge_create()`, `src/ratimos/theme.c:36`)
foi rejeitado em UAT ("esses icones estao feios porque tem essa bola vermelha redonda").
Vencedor: **ícone solto, sem nenhum container atrás** (sketch 002, variante B), com uma
**sombra em pixel dura** — offset de 2px, zero blur — em vez de um `drop-shadow` suave de
app web.

## Root cause técnica confirmada (não é só estética — é um bug de hitbox)

`ratimos_badge_create()` cria o badge circular via `lv_obj_create()`, que no LVGL vem
**clicável por padrão**. Sem handler próprio nem `LV_OBJ_FLAG_EVENT_BUBBLE`, o badge
intercepta o toque no centro do ícone — exatamente onde o olho mira primeiro — sem repassar
pro `row` pai, que é quem tem o `click_cb` real (`row_list.c:42-45`). Isso é a causa mais
provável do bug de UAT "tive que clicar umas 10 vezes pra entrar no conexo" (gap G-02.1-1 em
`.planning/phases/02.1-visual-identity-games/02.1-UAT.md`).

**Remover o badge (a mudança visual pedida) resolve o bug de hitbox de graça** — sem o
`lv_obj_create()` extra no meio, o toque cai direto no `row`/tile que já é clicável e correto.

## CSS pattern (sketch, referência visual)

```css
.icon-shadow img { filter: drop-shadow(2px 2px 0 rgba(0,0,0,0.7)); }
```

Também disponível como classe utilitária compartilhada em `sources/themes/default.css`:
`.pixel-shadow { filter: drop-shadow(2px 2px 0 rgba(0,0,0,0.7)); }`

## Path pra implementação real em LVGL

`lv_obj_set_style_shadow_width` no LVGL gera **blur**, não serve pra reproduzir esse efeito.
A forma real de fazer uma "sombra pixel" (offset duro, sem blur) em LVGL é desenhar um
**segundo `lv_image`** com o mesmo ícone, tingido de preto/escuro (`lv_obj_set_style_img_recolor`
+ `img_recolor_opa`), posicionado 2px à direita e 2px abaixo do ícone principal, atrás dele
na ordem z. Ou aceitar que essa sombra é puramente cosmética e pode ficar pra uma iteração
posterior de polish, priorizando primeiro remover o badge (que já resolve o bug funcional).

## Uso do mesmo tratamento no logo do sistema

O mesmo `.pixel-shadow` (ou seu equivalente LVGL) deve ser aplicado ao ícone de logo do
sistema no topbar (ver `header-navegacao.md`) — é o mesmo peão-torre vermelho, mesma família
de sombra, pra manter consistência.

## What to Avoid

- Manter o `lv_obj_create()` do badge só trocando a cor/formato — não resolve o bug de
  hitbox, só disfarça o sintoma visual.
- Sombra com blur (`box-shadow`/`drop-shadow` com raio > 0) — foi explicitamente rejeitada
  ("sombra mais em pixel, não tão smooth/suave").

## Origin

Synthesized from sketch: 002 (winner: B, refinado com sombra pixel dura)
Source files available in: `sources/002-icon-no-badge/index.html`
