---
sketch: 010
name: temas
question: "Temas que juntam animação de boot + fundo do boot + fundo do sistema: quais 4 e como escolher em Configurações?"
winner: "4 temas + carregamento separado"
tags: [theme, boot, background, settings]
---

# Sketch 010: Temas do RatimOS

## Design Question
Em vez de configurar a tela de boot e o fundo separados, cada **tema** junta a animação de boot
(sketch 008), o fundo do boot e o fundo da home (sketch 009). Os 4 temas e a tela de escolha funcionam?

## How to View
google-chrome .planning/sketches/010-temas/index.html   (aba "Escolher tema" ou `#flow`)

## Modelo (revisão 2, feedback da usuária)
**Tema e carregamento são escolhas independentes.** Tema = fundo do boot + fundo da home.
Carregamento = uma das 4 animações do 008 (bandeira, padrão; ameias; letreiro; torres).
Qualquer tema combina com qualquer carregamento.

| Tema | Fundo do boot | Fundo da home |
|------|---------------|---------------|
| **Clássico** (padrão) | `bg_dither.png` escurecido 55% | `bg_dither.png` |
| Noite estrelada | céu + estrelas piscando + lua | o mesmo céu, parado |
| Castelo ao entardecer | sol listrado com halo atrás da serra (esq.), castelo com telhados cônicos vermelhos, bandeirinhas e janelas piscando (dir.), tudo embaixo | mesma cena, horizonte em 450 |
| Xadrez | tabuleiro em perspectiva por pixel, com névoa no horizonte (sem moiré) e rolagem lenta | horizonte mais alto, parado |

## Configurações
`./home/config` ganha 2 linhas que dá para tocar: **tema** → `./home/config/tema` e **carregamento** →
`./home/config/carregamento`. Cada lista tem miniatura, "em uso", **prévia** (boot → home sem salvar) e
**aplicar** (salva em NVS).

## Outros ajustes pedidos
- Marca da topbar: "RatimOS", não "RATIMOS" (`status_bar.c:146` hoje usa "RATIMOS").

## Notas de implementação
- Os fundos da home são estáticos: no ESP32 vira uma imagem 320×480 por tema (como o `bg_dither.png`).
  Só o boot anima o fundo (estrelas, janelas, tabuleiro).
- Fundo diferente de preto exige o logo com alfa (o `logo_image.c` atual tem fundo preto opaco).
- 4 imagens de fundo 320×480 em RGB565 = ~300 KB cada na flash. Dá para gerar por código (estrelas,
  faixas, tabuleiro) em vez de guardar imagem, se a flash apertar.
