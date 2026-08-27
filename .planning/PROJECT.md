# RatimOS

## What This Is

RatimOS é um sistema operacional e dispositivo tipo "celular retrô" construído do zero como presente único e pessoal para a namorada do desenvolvedor. Roda em uma placa ESP32-S3 com tela touch, com apps de jogos, música, álbum de fotos, cartas e configurações, bateria própria, relógio, câmera e uma estética visual autoral (inspirada, mas não copiada, de um projeto de referência de terceiros conhecido como "colombiaOS").

## Core Value

O dispositivo tem que funcionar de verdade no dia a dia dela — offline, com as 5 seções (jogos, música, álbum, cartas, config) estáveis — e continuar "vivo" depois de entregue, recebendo cartas/fotos/músicas novas e atualizações de firmware remotamente, sem o desenvolvedor precisar pegar o aparelho de volta.

## Requirements

### Validated

(Nenhum ainda — hipóteses até serem implementadas e validadas no hardware real)

### Active

- [ ] Firmware RatimOS roda localmente (offline) as 5 seções: jogos, música, álbum, cartas, config
- [ ] Tela touch funciona com caneta/stylus, não só com o dedo
- [ ] Dispositivo tem bateria própria com gestão de carga/descarga e relógio (RTC) persistente
- [ ] Câmera integrada captura fotos que aparecem no álbum
- [ ] Dispositivo conecta ao wifi (provisionamento sem senha hardcoded) e sincroniza cartas/fotos/músicas novas de um backend na nuvem
- [ ] Firmware recebe atualizações OTA remotamente, sem contato físico com o aparelho
- [ ] Segurança básica antes da entrega: Secure Boot + Flash Encryption, nenhuma credencial em texto plano, comunicação sempre via HTTPS com token único por device
- [ ] Estética visual (paleta, tipografia, ícones) é autoral — inspirada na referência, não copiada

### Out of Scope

- PCB customizado desenhado do zero — usamos placa comercial pronta (Waveshare ESP32-S3-Touch-LCD-3.5) pra não empilhar aprendizado de design de PCB em cima de tudo mais
- Emuladores de console (NES, GBA etc.) — jogos leves de tabuleiro/cartas (sudoku, paciência) são muito mais viáveis nesse hardware e resolvem o pedido original
- Backend rodando em máquina própria do desenvolvedor — precisa ficar sempre acessível pra sincronizar mesmo sem o PC ligado, então vai pra nuvem (baixo custo/gratuito)
- Data de entrega fixa — sem pressão de prazo, fases sequenciais

## Context

- Desenvolvedor nunca mexeu com eletrônica embarcada antes; é o único mantenedor do projeto.
- Ainda não existe hardware físico — o projeto começou 100% em software, testável via simulador LVGL/SDL2 no PC antes de qualquer compra.
- As fotos de referência mostram um projeto de terceiros ("colombiaOS") rodando num ESP32 clássico com painel touch resistivo genérico (família "TPM408") e um D-pad soldado numa PCB customizada — serviu de inspiração de estética e estrutura de menu, não é hardware do usuário.
- Pesquisa de hardware já feita e decidida: telas 4.3"/4.8" de baixo custo (RGB paralelo) não sobram GPIO pra câmera no ESP32-S3; a Waveshare 3.5" usa QSPI e tem interface de câmera onboard, RTC dedicado (PCF85063), codec de áudio dedicado (ES8311), PMIC (AXP2101) — por isso foi a escolha.
- Já existe um shell inicial em PlatformIO + LVGL rodando no simulador SDL2 do PC (Fase 0 do plano original), com as 5 telas navegáveis (ainda sem lógica real).
- **Fase 1 completa (2026-08-27):** shell agora roda de ponta a ponta no simulador — splash de boot real com a logo oficial (fade-in + barra de progresso ligada à indexação de conteúdo), navegação home↔5 seções, e todos os 5 apps lendo exclusivamente através de uma Storage/Content API compartilhada (não mais dados hardcoded). HAL de board separado (`board_native_sdl` real, `board_waveshare_s3_35` como stub que compila) prova que o código dos apps compila igual pros dois lados antes do hardware chegar. Conteúdo dos apps ainda é 100% fixture/placeholder (jogos não são jogáveis de verdade, fotos/músicas/cartas são mock) — isso é esperado, real virá nas Fases 5-7.

## Constraints

- **Tech stack**: ESP32-S3 + PlatformIO + Arduino core + LVGL — escolhido por ser o caminho mais amigável pra quem nunca fez desenvolvimento embarcado, com possibilidade de migrar partes pra ESP-IDF puro depois se precisar de mais controle.
- **Orçamento**: baixo custo — placa principal ~R$330-360, mais bateria LiPo, cartão microSD e módulo de câmera OV5640 se não vier incluso.
- **Timeline**: sem prazo fixo — o presente é entregue quando o hardware estiver pronto e estável, não numa data específica.
- **Hardware disponível**: nenhum ainda — todo desenvolvimento inicial precisa ser testável em simulador de PC (SDL2) antes da placa chegar.
- **Segurança**: dispositivo vai ficar conectado à rede wifi pessoal dela — não pode virar porta de entrada pra rede doméstica nem expor dados íntimos (fotos, cartas) sem autenticação adequada.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Chip ESP32-S3 (não ESP32 clássico) | Mais RAM/PSRAM, suporta câmera nativamente, folga pra rodar LVGL + várias apps simultâneas | — Pending |
| Placa Waveshare ESP32-S3-Touch-LCD-3.5 (não a 4.3" cogitada inicialmente) | Telas 4.3"+ nesse segmento usam barramento RGB paralelo que consome quase todos os GPIOs, impossibilitando câmera onboard; a 3.5" usa QSPI e sobra GPIO pra câmera, RTC e áudio dedicados | — Pending |
| Câmera incluída já no MVP (não adiada pra fase 2) | Decisão explícita do usuário, apesar do custo/complexidade adicional | — Pending |
| Conectividade online com backend na nuvem (não modo local-only) | Requisito central do usuário: continuar mandando conteúdo novo e atualizações depois de entregar o presente, sem acesso físico ao aparelho | — Pending |
| Kit comercial pronto em vez de PCB customizado do zero | Usuário não tem experiência em eletrônica; reduz uma dimensão inteira de risco/aprendizado (design de PCB, soldagem SMD) | — Pending |
| PlatformIO + Arduino core + LVGL (não ESP-IDF puro de cara) | Curva de aprendizado mais suave pra quem nunca fez embarcado; dá pra migrar partes pra ESP-IDF depois se precisar | — Pending |
| Paleta de cores oficial derivada da logo real (D-17, Fase 1), adiantada da Fase 9 | Usuário trouxe a arte final da logo (torre/xadrez + "RatimOS" em gradiente roxo/vermelho) e pediu pra já repintar o sistema inteiro agora em vez de esperar a fase de identidade visual | Implementado — fundo `#000000`, acento `#e6010f`, tons violeta `#2a123f`/`#4e2277`, amostrados por pixel da logo |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-08-27 after Phase 1 completion*
