---
sketch: 004
name: header-breadcrumb
design_question: Como deve ficar o header (logo + título de seção), trocando "home.mem" por um caminho tipo explorador de arquivos (./home/jogos/conexo) e adicionando o relógio?
winner: "A" (refinado: sem emoji, logo = peão-torre vermelho, sem cursor piscando)
tags: [header, navigation, clock]
---

## Variants

- **A — Duas barras, breadcrumb mono + relógio:** mantém topbar/sectionbar atuais, título vira caminho crescente (`./home` → `./home/jogos` → `./home/jogos/conexo`), cursor piscando no fim, relógio real (RTC, quando existir) no topbar.
- **B — Prompt de terminal:** barra única estilo shell (`ela@ratimos:~/home/jogos/conexo$`), mais "nerd retro"; risco de truncar em caminhos profundos.
- **C — Breadcrumb em "pills":** cada segmento do caminho vira uma pill, atual em vermelho-marca; mais visual, mas aperta em 3 níveis de profundidade numa tela de 300px.

## Notes

Sobre native_sim, o relógio é mockado (não há RTC real ainda — chega só na Fase 4, Power
Management). Nesta fase o relógio pode usar hora do sistema PC ou ficar com um placeholder
até a Fase 4.
