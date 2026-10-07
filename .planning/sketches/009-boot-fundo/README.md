---
sketch: 009
name: boot-fundo
question: "O que vai atrás do logo na tela de boot (hastear a bandeira, 008-B) sem tirar o destaque da marca?"
winner: "incorporado ao 010"
tags: [boot, splash, background, brand]
---

# Sketch 009: Fundo da tela de boot

## Design Question
A tela de boot 008-B ficou só com fundo preto. Qual fundo dá mais vida sem competir com o logo?

## How to View
google-chrome .planning/sketches/009-boot-fundo/index.html

## Variants
- **A: Céu estrelado**: céu noturno roxo escuro, estrelas de 1 px piscando e uma lua pixel.
- **B: Degradê do sistema**: o `bg_dither.png` da home escurecido 55%. O boot já entra "dentro" do sistema.
- **C: Pôr do sol no castelo**: céu em faixas nas cores do letreiro, sol pixel, colinas e a silhueta de uma muralha com janelinhas acesas.
- **D: Tabuleiro de xadrez**: tabuleiro roxo em perspectiva deslizando, já que a torre é uma peça de xadrez.
- **Preto (atual)**: para comparar.

## Nota técnica
`logo_image.c` tem fundo preto opaco. Qualquer fundo que não seja preto exige um logo com alfa
(o sketch usa `logo_256_alpha.png`, com preto < 14 transparente) ou chroma key no LVGL. Custo extra: ~1 bit/px de alfa, ou
ARGB8565 no lugar de RGB565.
