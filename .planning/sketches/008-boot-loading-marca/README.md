---
sketch: 008
name: boot-loading-marca
question: "Qual barra de carregamento do boot conversa de verdade com o símbolo do RatimOS (torre vermelha + bandeira + letreiro em degradê)?"
winner: "B"
tags: [boot, splash, loading, brand]
---

# Sketch 008: Barra de boot casada com a marca

## Design Question
A barra atual (25 blocos no bevel 003-C, sketch 007-B) funciona, mas parece uma peça genérica
colada embaixo do logo. A pergunta é qual indicador usa os elementos do próprio símbolo
(torre, bandeira, letreiro em degradê roxo→vermelho→laranja) para virar uma peça só com o logo.

## How to View
open .planning/sketches/008-boot-loading-marca/index.html

Canvas 1:1 com a tela real 320×480, com botão de zoom 2×. Todas as variantes usam os
pixels reais de `logo_256.png` e do `assets/icons/system_rook.png` (14×20), sem redesenhar a marca.

## Variants
- **A: Ameias do castelo**: a barra é o topo de uma muralha, da largura exata do letreiro, preenchida tijolo a tijolo com o degradê vertical do "RatimOS". No fim, a bandeira é fincada na última ameia.
- **B: Hastear a bandeira**: sem barra. O mastro da torre fica mais alto e a bandeira do logo sobe 2 px por vez, tremulando.
- **C: Letreiro que acende**: o logo começa apagado e o letreiro acende da esquerda pra direita com um friso de brilho. A torre acende no fim.
- **D: Fileira de torres**: 6 torrinhas do ícone do sistema enchem de baixo pra cima (como os corações do Zelda), tingidas do roxo ao laranja.
- **Atual (referência)**: aproximação do que está no `splash.c` hoje.

## What to Look For
- Qual parece "parte do logo" e qual parece "componente colado embaixo"
- Leitura do progresso em 1 segundo de olhada (B e C são mais sutis que A e D)
- O momento "pronto!" (bandeira fincada, torre acendendo, último pulinho)
- Custo no LVGL: A, B e D são retângulos e sprites num draw callback; C precisa do logo em duas versões (apagada e acesa) ou de recorte por clip area

## Referências
- Barras pixel art clássicas, com LOADING, blocos e moldura (Adobe Stock, packs "8-bit loading bar"), já cobertas no 007-B
- Corações do Zelda / HP segmentado do Mega Man → D
- Mastro de fim de fase do Mario → B
- Brilho que varre o logo no boot do Game Boy/Sega → C

## Decisão (2026-10-07)
- **Vencedora: B (Hastear a bandeira)**, a tela de boot padrão. Ajustes feitos: sem as marcações de 25% ao lado do mastro, e o mastro remontado sem emendas (ponta + sombra sob a ponta + linha esticada até o telhado).
- **A, C e D não foram descartadas:** viram telas de entrada alternativas, escolhidas pela usuária em Configurações. B é o padrão.
