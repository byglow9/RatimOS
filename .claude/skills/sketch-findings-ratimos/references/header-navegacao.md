# Header & Navegação

## Design Decisions

O título de seção atual (ex.: "home.mem", texto simples no `sectionbar`) foi apontado como
"fonte horrível" e "não combina com o sistema". Pedido: trocar por um **caminho estilo
explorador de arquivos**, crescendo com a profundidade da navegação (`./home` →
`./home/jogos` → `./home/jogos/conexo`), mais um **relógio** no topbar. Vencedor entre 3
variantes (sketch 004): **A — duas barras, breadcrumb mono**, refinada após feedback
adicional pra remover emoji e o cursor piscando.

## Anatomia da variante A (vencedora, já refinada)

Duas barras (mantém a estrutura existente `topbar` + `sectionbar`, não fundir em uma só —
isso era a variante B, rejeitada):

- **Topbar:** logo do sistema (peão-torre vermelho, não mais o emoji 🏠) + nome "RATIMOS" +
  relógio + indicador de bateria em **ícone pixel-art**, não emoji `🔋`.
- **Sectionbar:** caminho crescente, segmentos-pai em cinza (`--ratimos-text-muted`), segmento
  atual em texto normal (`--ratimos-text`). **Sem cursor piscando** — a primeira versão tinha
  um cursor de terminal animado (`@keyframes blink`), removido a pedido explícito.

```css
.sectionbar-a .seg-muted { color: var(--ratimos-text-muted); }
.sectionbar-a .seg-current { color: var(--ratimos-text); font-weight: 600; }
```

Exemplos de caminho por profundidade:
```
./home
./home/jogos
./home/jogos/conexo
```

## Logo do sistema: peão-torre vermelho

A usuária forneceu uma imagem de referência (peão-torre vermelho com bandeira, estilo
pixel-art) pra ser o ícone que representa o sistema — substitui totalmente o emoji 🏠.
Processado (chroma-key removendo fundo cinza `#212121` sólido, crop, downscale) em
`sources/themes/system_rook_32.png` (32px de largura, PNG com alpha real). Aplica-se a
mesma sombra em pixel (ver `icones.md`) a esse ícone no topbar.

```css
.topbar-a .logo img { height: 15px; width: auto; image-rendering: pixelated; filter: drop-shadow(1px 1px 0 rgba(0,0,0,0.7)); }
```

## Bateria em pixel-art (substitui o emoji 🔋)

```css
.pixel-battery { display:inline-flex; align-items:center; gap:2px; }
.pixel-battery .cell { width: 16px; height: 8px; border: 1px solid var(--ratimos-text); position: relative; }
.pixel-battery .cell::after { content:""; position:absolute; right:-3px; top:2px; width:2px; height:2px; background: var(--ratimos-text); } /* terminal do polo positivo */
.pixel-battery .fill { position:absolute; left:1px; top:1px; bottom:1px; background: var(--ratimos-game-correct); width: 70%; } /* nível de carga, dinâmico */
```

`.fill` width é o dado real de carga (%) — vem do PMIC AXP2101 via `XPowersLib` quando a
Fase 4 (Power Management) estiver pronta; até lá, mockar um valor fixo ou omitir o
preenchimento dinâmico.

## Relógio

Relógio real depende do RTC PCF85063 (Fase 4 — Power Management), fora do escopo desta
fase/rework. Na Fase 02.1 (native_sim), usar a hora do sistema PC como placeholder, ou um
valor estático — não bloquear o rework visual esperando hardware.

## Nota sobre truncamento

Não testado em profundidade nesta sessão: caminhos com 3+ segmentos (`./home/jogos/conexo`)
em painel de 300px úteis. As variantes B (prompt) e C (pills) do sketch 004 sinalizaram esse
risco explicitamente; a variante A vencedora usa fonte mono pequena (11px) que no teste visual
coube nos 3 exemplos mostrados, mas vale confirmar contra o nome mais longo real do projeto
("cruzadinha", "paciência") antes de travar como não-problema.

## What to Avoid

- Qualquer emoji na UI (🏠, 🔋, etc.) — pedido explícito, "quero só coisa retro".
- Cursor piscando na sectionbar — testado e rejeitado.
- Fundir topbar+sectionbar numa barra única estilo terminal (variante B) — não foi o
  escolhido.

## Origin

Synthesized from sketch: 004 (winner: A, refinado)
Source files available in: `sources/004-header-breadcrumb/index.html`, `sources/themes/system_rook_32.png`
