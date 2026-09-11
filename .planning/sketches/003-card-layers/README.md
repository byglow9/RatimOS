---
sketch: 003
name: card-layers
design_question: Como o card/linha (jogos, home, config etc.) deve ficar — sem cantos arredondados, levemente transparente, com título + descrição mais claros?
winner: "C" — moldura bevel retrô (GameBoy/Win95), translúcida
tags: [card, typography]
---

## v2 (2026-09-11) — reconstruído a partir do feedback

A v1 (quadriculado + camadas com cantos arredondados) foi rejeitada em bloco — nenhuma das 3
opções agradou. Pedido explícito: sem `border-radius`, levemente transparentes, mais opções
(5) pra escolher.

## Variants

- **A — Quadriculado translúcido:** textura quadriculada com alpha (~70-88%), cantos retos, borda fina vermelho-marca.
- **B — Vidro fosco (glass):** painel ~42% opaco + blur atrás, sem textura. Nota: LVGL não tem blur real de backdrop — versão de produção seria um flat semi-transparente sem o blur.
- **C — Moldura bevel retrô (GameBoy/Win95):** borda dupla (externa escura + friso interno claro via inset box-shadow), sem textura, mais barato de implementar no LVGL.
- **D — Scanlines translúcidas:** linhas horizontais finas de 1px sobre painel translúcido, ecoa o fundo (sketch 001-C) sem repetir o quadriculado.
- **E — Camadas + canto cortado (chanfro):** mantém o efeito de profundidade em camadas da v1, mas troca curva por corte reto de canto (técnica clássica de pixel-art), translúcido.

## Notes

Todas as 5 já usam a decisão travada de tipografia (fonte pixel pro título + descrição mono
abaixo) e o ícone/sombra-pixel do sketch 002 (ganhador B). Fundo do device frame usa o
vencedor do sketch 001 (variante C, dithered + scanline).
