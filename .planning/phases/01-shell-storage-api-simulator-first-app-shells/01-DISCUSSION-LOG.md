# Phase 1: Shell, Storage API & Simulator-First App Shells - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-08-26
**Phase:** 1-Shell, Storage API & Simulator-First App Shells
**Areas discussed:** Boot Identity / Splash, HAL / Board Abstraction, Storage/Content API, Conteúdo mockado no simulador

---

## Boot Identity / Splash

| Option | Description | Selected |
|--------|-------------|----------|
| Logo + nome, estático | "RatimOS" centralizado com acento teal, sem animação | |
| Logo + nome, com fade-in | Mesmo visual, com animação de entrada suave | ✓ |
| Logo + tagline | Nome + frase/assinatura pessoal abaixo | |

**User's choice:** Logo + nome, com fade-in

**Notes:** Na pergunta seguinte sobre duração, o usuário pediu uma "barriga de carregamento" real (~2s) e sugeriu que a tela de boot já checasse wifi/atualização/conteúdo novo — redirecionado como scope creep (pertence às Fases 6/7/8) e capturado em Deferred Ideas.

| Option | Description | Selected |
|--------|-------------|----------|
| Progresso real da Storage API | Barra avança conforme passos reais de boot (tema → Storage API → indexação) | ✓ |
| Progresso simulado por tempo | Barra anima 0-100% em ~2s sem estar amarrada a passos reais | |

**User's choice:** Progresso real da Storage API

| Option | Description | Selected |
|--------|-------------|----------|
| Sim, toque pula | Toque leva pra home antes do timer acabar, se já carregou | |
| Não, sempre completa | Toque ignorado durante o splash | ✓ |

**User's choice:** Não, sempre completa

---

## HAL / Board Abstraction

| Option | Description | Selected |
|--------|-------------|----------|
| Mínimo: init + tick | `board_init()` + `board_tick()` | |
| Explícito: display + input separados | `board_display_init()`, `board_input_init()`, `board_tick()` | ✓ |

**User's choice:** Explícito: display + input separados

| Option | Description | Selected |
|--------|-------------|----------|
| src/board/native_sdl/, src/board/waveshare_s3_35/ | Pasta src/board/ dedicada, subpasta por board | |
| src/ratimos/board_native_sdl.c + board_waveshare_s3_35.c | Junto do resto do código RatimOS | |

**User's choice:** Delegado a Claude ("você que decide isso, onde for melhor mais organizado com mais desempenho e escalável")
**Notes:** Decidido: `src/board/native_sdl/` e `src/board/waveshare_s3_35/` — separa código de hardware do código de app.

| Option | Description | Selected |
|--------|-------------|----------|
| Esqueleto compilando (stub) | Implementa a interface com TODOs, compila sem funcionar de verdade | ✓ |
| Não criar ainda | Só board_native_sdl agora | |

**User's choice:** Esqueleto compilando (stub)

| Option | Description | Selected |
|--------|-------------|----------|
| main.c genérico e único | Um main.c pros dois ambientes, zero código SDL2 específico | ✓ |
| main.c por board | Um main.c pra cada ambiente | |

**User's choice:** main.c genérico e único
**Notes:** Usuário pediu explicação em linguagem simples antes de confirmar ("qual a diferença? fale de uma forma mais simples").

---

## Storage/Content API

| Option | Description | Selected |
|--------|-------------|----------|
| Fotos, faixas, cartas, jogos-lista | 4 tipos de conteúdo, config fica fora | |
| Os 4 acima + settings do config | Mesma coisa + settings (brilho, volume, wifi placeholder) | ✓ |

**User's choice:** Os 4 acima + settings do config

| Option | Description | Selected |
|--------|-------------|----------|
| Síncrona agora, assíncrona depois | Chamadas síncronas simples na Fase 1; async só quando SD/rede real exigirem | ✓ |
| Já assíncrona por callback | Toda leitura já usa callback/evento desde a Fase 1 | |

**User's choice:** Síncrona agora, assíncrona depois

| Option | Description | Selected |
|--------|-------------|----------|
| src/ratimos/storage/ (novo módulo) | Dentro de ratimos/, junto da UI | |
| src/storage/ (mesmo nível de board/) | Camada de infraestrutura, irmã de board/ | ✓ |

**User's choice:** src/storage/
**Notes:** Usuário pediu explicação simples da diferença antes de confirmar.

---

## Conteúdo mockado no simulador

| Option | Description | Selected |
|--------|-------------|----------|
| Dados fake fixos no código | Array hardcoded em C dentro do backend nativo | |
| Fixtures em disco (pasta assets/) | Arquivos reais (jpgs, mp3s, txt) lidos do disco do PC | ✓ |

**User's choice:** Fixtures em disco (pasta assets/)

| Option | Description | Selected |
|--------|-------------|----------|
| Poucos (3-4 por tipo) | Mínimo pra ver grid/lista funcionando com mais de um item | ✓ |
| Mais variado (8-10 por tipo) | Melhor pra testar scroll/paginação de verdade | |

**User's choice:** Poucos (3-4 por tipo)

| Option | Description | Selected |
|--------|-------------|----------|
| Genérico/placeholder | Conteúdo real só entra depois via sync (Fase 7) | ✓ |
| Já com toque pessoal | Fixture real selecionada pelo usuário | |

**User's choice:** Genérico/placeholder
**Notes:** Reforçado pelo fato de as fixtures ficarem no repositório de código, já publicado no GitHub durante esta sessão.

| Option | Description | Selected |
|--------|-------------|----------|
| assets/mock/ na raiz do projeto | Pasta assets/mock/ só lida pelo backend nativo no ambiente native_sim | ✓ |
| src/storage/native_sdl/fixtures/ | Junto do próprio backend nativo do storage | |

**User's choice:** assets/mock/ na raiz do projeto

---

## Claude's Discretion

- Layout exato de arquivos dentro de `src/board/native_sdl/` e `src/board/waveshare_s3_35/` (quantos arquivos por board, split header/source) — delegado explicitamente pelo usuário na área HAL / Board Abstraction.

## Deferred Ideas

- Splash de boot verificando wifi (auto-conectar em rede já conhecida) e checando atualização/conteúdo novo (OTA/sync) durante o carregamento — pertence às Fases 6 (WiFi Provisioning), 7 (Cloud Content Sync) e 8 (OTA), não à Fase 1.
