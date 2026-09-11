# Cards & Superfícies

## Design Decisions

O painel atual (`ratimos_panel_create()`, `src/ratimos/theme.c:13-25`) — fundo roxo sólido
`RATIMOS_COLOR_PANEL` opaco, `radius: 6px`, borda 1px vermelha — foi rejeitado em bloco na
primeira rodada de esboços (3 variantes com quadriculado + cantos arredondados, nenhuma
aprovada). Pedido explícito: **sem `border-radius` nenhum**, **levemente transparente**.

Segunda rodada (sketch 003 v2, 5 variantes, todas com cantos retos e translucidez) —
vencedor: **variante C — moldura dupla estilo bevel retrô (GameBoy / Windows 95)**.

## Anatomia da variante C (vencedora)

- Cantos 100% retos (`border-radius: 0`)
- Fundo roxo translúcido liso — **sem** textura/quadriculado (diferente da v1 e da variante A)
- Borda externa escura de 2px (`#0d0515` no sketch)
- Friso interno claro de 1px, feito com `box-shadow: inset` em duas camadas (não uma segunda
  borda real — ver CSS abaixo)
- `padding: 9px`

```css
.card-c {
  border-radius: 0;
  padding: 9px;
  background: rgba(42,18,63,0.68);       /* RATIMOS_COLOR_PANEL a ~68% de opacidade */
  border: 2px solid #0d0515;
  box-shadow:
    inset 0 0 0 1px rgba(255,255,255,0.15),
    inset 0 0 0 3px rgba(0,0,0,0.35);
}
```

Título em fonte pixel (Heading, D-08) + linha de descrição em mono abaixo (ambos já decisões
travadas do projeto, D-08/UI-SPEC — este sketch só confirma que a hierarquia "nome + descrição
curta" funciona dentro do novo card).

```css
.row-inner .t { font-family: var(--font-display); font-size: 9px; } /* mapeia pro Heading 16px real do LVGL */
.row-inner .d { font-family: var(--font-mono); font-size: 9px; color: var(--ratimos-text-muted); }
```

## Por que C venceu sobre as outras 4 (A quadriculado translúcido, B vidro fosco, D scanlines, E camadas+chanfro)

Feedback direto da usuária apontando a screenshot da variante C como favorita — "esse forem
meus favoritos". Não há registro de por que especificamente rejeitou as outras 4 nesta
sessão; se surgir dúvida durante a implementação, a leitura mais segura é que o bevel duplo
lê como "hardware/interface retrô real" (GameBoy/Win95) de um jeito que quadriculado/vidro/
scanline não igualam, e é also o mais barato de implementar (2 bordas, sem imagem/textura).

## Path pra implementação real em LVGL

- `lv_obj_set_style_radius(panel, 0, 0)` — substitui o `6` atual em `ratimos_panel_create()`.
- `lv_obj_set_style_bg_opa(panel, LV_OPA_70, 0)` (aprox. — calibrar contra o fundo real depois
  que o bg dithered (ver `fundo-e-ambiente.md`) estiver implementado, já que a opacidade foi
  calibrada visualmente contra aquele fundo específico).
- Borda externa: `lv_obj_set_style_border_width(panel, 2, 0)` + `lv_obj_set_style_border_color(panel, lv_color_hex(0x0d0515), 0)`.
- Friso interno: LVGL não tem `box-shadow: inset` nativo. Aproximação real: um segundo
  `lv_obj` filho, `width/height = 100% - 2px` de margem, sem fundo, só com borda 1px branca
  translúcida (`lv_opa` baixa) — ou aceitar uma única borda mais grossa como fallback mais
  simples se o segundo objeto não couber no orçamento de objetos por tela.

## What to Avoid

- Qualquer `border-radius` > 0 nos cards — decisão explícita e repetida (rejeitada duas vezes
  antes de fechar em cantos retos).
- Quadriculado/textura de fundo no card em si (isso foi pra fundo de tela, não pro card —
  ver `fundo-e-ambiente.md`; misturar os dois fica carregado, como o próprio sketch 003-D
  já sinalizava como risco).
- Opacidade alta o bastante pra virar opaco de novo — o pedido foi "levemente transparente".

## Origin

Synthesized from sketch: 003 v2 (winner: C)
Source files available in: `sources/003-card-layers/index.html`
