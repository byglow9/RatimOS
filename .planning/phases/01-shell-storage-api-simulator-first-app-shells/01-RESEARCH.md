# Phase 1: Shell, Storage API & Simulator-First App Shells - Research

**Researched:** 2026-08-26
**Domain:** LVGL v9.5 app-shell architecture on PlatformIO `native` (SDL2 simulator), board/HAL abstraction, synchronous local content API, PlatformIO native unit testing
**Confidence:** HIGH (core LVGL APIs read directly from the installed library source in this repo's `.pio/libdeps`; architecture patterns MEDIUM per prior project research; PlatformIO native-testing mechanics CITED from official docs)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Boot Identity / Splash**
- D-01: Splash shows the RatimOS logo/name with a fade-in animation (`lv_anim`).
- D-02: Splash lasts ~2s and shows a loading bar tied to real boot progress (not a fake timer).
- D-03: The loading bar reflects actual Storage/Content API initialization steps (mount native/mock backend → index mock content) — real feedback, not decoration.
- D-04: Touch during the splash does nothing — it always plays to completion, no tap-to-skip.

**HAL / Board Abstraction**
- D-05: Board interface contract is explicit: `board_display_init()`, `board_input_init()`, `board_tick()` — separate functions, not a single monolithic init. This anticipates Phase 3, where display (ST7796) and touch (FT6336) will have genuinely different init/failure paths.
- D-06: Board implementations live in `src/board/native_sdl/` and `src/board/waveshare_s3_35/` — one subfolder per board, sibling structure. — Reversibility: costly — every `#include`/build filter referencing the old flat-file layout would need updating if this changes later.
- D-07: `board_waveshare_s3_35` is a compiling stub in Phase 1 (implements the interface with TODOs, no real hardware behavior). No `esp32s3` PlatformIO environment is added yet — `platformio.ini` still only builds `native_sim`; the stub just needs to compile as a translation unit to prove the interface holds. Wiring an actual build target for it is out of scope for this phase.
- D-08: `main.c` stays a single, generic entry point for both environments — it only calls the board interface functions, with zero SDL2-specific code inline. This is what lets app code "compile unchanged against both" per the ROADMAP's Phase 1 success criteria.

**Storage/Content API**
- D-09: The API covers 5 content domains: photos (album), tracks (musica), letters (cartas), games list (jogos), and settings (config — brightness/volume/device info placeholders). Settings were pulled into the same API rather than left hardcoded in `config_app.c`, so config isn't a special case.
- D-10: The API is synchronous in Phase 1 (e.g. `ratimos_storage_list_photos()` returns directly). Async (callback/event-based) is explicitly deferred to whichever phase first needs it for real (SD I/O latency in Phase 5, network sync in Phase 7) — do not build async plumbing now, it would be premature for a synchronous mock backend.
- D-11: Lives in `src/storage/` — a sibling of `src/board/`, not nested under `src/ratimos/`. Signals it's a shared infrastructure layer serving all apps equally, not part of the UI layer.

**Conteúdo mockado no simulador**
- D-12: The native storage backend reads real fixture files from disk (not hardcoded C arrays) — closer to how real SD reads will behave once Phase 5 lands.
- D-13: Keep fixture counts small: 3-4 items per content type (photos, tracks, letters, games) — enough to prove list/grid rendering with more than one item, without bloating the repo.
- D-14: Fixture content must be generic/placeholder (e.g. "Carta de teste 1", stock-style placeholder images) — not real personal content. These fixtures live in the git repository (already published to GitHub), so nothing personal belongs here. Real content only ever arrives via cloud sync starting Phase 7.
- D-15: Fixtures live at `assets/mock/` at the project root — read only by the native storage backend when built for `native_sim`. The name makes it unambiguous this is dev-only and must never be pulled into an `esp32s3` build.

### Claude's Discretion
- Exact file layout inside `src/board/native_sdl/` and `src/board/waveshare_s3_35/` (how many files per board, header/source split) — user explicitly delegated this ("você que decide isso, onde for melhor mais organizado com mais desempenho e escalável"). Recommendation from CONTEXT.md: mirror the existing `src/ratimos/*.c/.h` pattern per board (e.g. `board.c`/`board.h` implementing the shared interface).

### Deferred Ideas (OUT OF SCOPE)
- Boot-time wifi auto-connect + OTA/sync check on the splash screen — explicitly out of scope for Phase 1 (simulator-only, no network stack exists yet). Belongs across Phase 6 (WiFi Provisioning), Phase 7 (Cloud Content Sync), and Phase 8 (OTA).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|--------------------|
| SHELL-01 | User can navigate from home to any of the 5 sections (jogos/musica/album/cartas/config) and back | Already implemented in `src/ratimos/home_screen.c`/`app_shell.c`/`status_bar.c` (verified this session) — Phase 1's job is to keep this working while apps switch to reading through the Storage API; see Architecture Patterns and Recommended Project Structure. No navigation logic changes required, only content-source changes inside each `apps/*_app.c`. |
| SHELL-03 | Device boots directly into the home screen with a RatimOS boot identity, not a stock/blank screen | Addressed by the Splash-Driven Staged Storage Init pattern (Pattern 2) plus `lv_obj_fade_in`/`lv_bar` verified LVGL APIs (Standard Stack, Don't Hand-Roll) satisfying D-01 through D-04. |
</phase_requirements>

## Summary

Phase 1 refactors an existing, working Phase-0 SDL2/LVGL shell (`src/main.c` + `src/ratimos/*`) into three cleanly separated layers — **board/HAL**, **Storage/Content API**, **app/shell UI** — without touching the UI's visual behavior. The existing code already follows good habits (a shared `app_shell.c` chrome, `theme.c` design tokens, `row_list.c` shared rows) that should be reused, not rewritten. The two real risks in this phase are (1) `main.c` and `src/main.c`'s current SDL2 calls (`lv_sdl_window_create`, `lv_sdl_mouse_create`, `lv_tick_set_cb(SDL_GetTicks)`, `SDL_Delay`) are NOT currently behind any abstraction — every one of them must move into `board/native_sdl/`, leaving `main.c` with zero `#include <SDL2/SDL.h>`; and (2) the splash's "real progress, ~2s duration, no fake timer" requirement (D-02/D-03) is only satisfiable cleanly if storage init is broken into discrete, named steps driven by an `lv_timer`, not a single blocking synchronous call — this reconciles "real feedback" with "predictable ~2s pacing" without violating D-10 (no async plumbing).

The Storage/Content API (`src/storage/`) should be a plain, allocation-free C API — fixed-size caller-provided output arrays over dynamic lists, given the fixture counts are 3-4 items per domain (D-13) and the project's ESP32 target has limited heap. Only ONE concrete storage backend exists this phase (native/mock reading `assets/mock/`); do not build a backend-selection abstraction (interface + registered implementations) for storage the way `board/` needs one — `board/` genuinely has two concrete implementations landing this phase (native_sdl + waveshare_s3_35 stub), storage does not (the SD backend is Phase 5's job). Building that seam prematurely repeats the same anti-pattern D-10 already tells you to avoid for async.

**Primary recommendation:** Extract `board_display_init()/board_input_init()/board_tick()` into `src/board/native_sdl/board.c` (moving every SDL2 call out of `main.c`), build `src/storage/` as direct allocation-free C functions per content domain (no backend-interface layer), and drive the splash's loading bar from an `lv_timer` that advances through named storage-init steps (mount → index photos → index tracks → index letters → index games → index settings), each step real work, paced to sum to ~2s.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Boot splash / identity | App/Shell (LVGL) | Board (tick source for anim timing) | Pure UI concern; only needs `lv_tick`/timer services from the board layer |
| Home ↔ 5-section navigation | App/Shell (LVGL) | — | `lv_screen_load` based, already implemented, no storage/board dependency |
| Display + input bring-up | Board/HAL | — | `board_display_init()`/`board_input_init()` are exactly the seam; SDL2 today, ST7796/FT6336 in Phase 3 |
| Main loop pacing / tick | Board/HAL | App/Shell (calls it each iteration) | `board_tick()` owns the platform-specific idle/delay (SDL_Delay vs FreeRTOS delay) so `main.c` stays generic |
| Content read (photos/tracks/letters/games/settings) | Storage/Content API | App/Shell (renders what it's given) | Single point of truth so apps never know if content came from a fixture file, real SD, or future cloud sync |
| Fixture file I/O (`assets/mock/`) | Storage/Content API (native backend) | — | Confined entirely inside `src/storage/`; no other layer touches `fopen`/`FS.h` |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| LVGL | 9.5.0 | UI toolkit, already vendored at `.pio/libdeps/native_sim/lvgl` | [VERIFIED: .pio/libdeps/native_sim/lvgl/lv_version.h:9-11] — `#define LVGL_VERSION_MAJOR 9` / `#define LVGL_VERSION_MINOR 5` / `#define LVGL_VERSION_PATCH 0`. Matches CLAUDE.md's recommended 9.5.x line. |
| PlatformIO `platform = native` | 6.1.19 (local pio CLI) | SDL2 simulator build target, already configured | [VERIFIED: platformio.ini:10-23] existing `[env:native_sim]` section; `pio --version` on this machine reports `PlatformIO Core, version 6.1.19`. |
| SDL2 | system-provided (`-lSDL2` link flag) | Window/mouse driver backing LVGL's SDL indev/display drivers in the simulator | [VERIFIED: platformio.ini:17] `-lSDL2` build flag; [VERIFIED: src/main.c:5] `#include <SDL2/SDL.h>` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| LVGL SDL driver (`lv_sdl_window`, `lv_sdl_mouse`) | bundled with LVGL 9.5 | Display + pointer input for the native_sdl board backend | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/drivers/sdl/lv_sdl_window.h:44] `lv_display_t * lv_sdl_window_create(int32_t hor_res, int32_t ver_res);` and [VERIFIED: .pio/libdeps/native_sim/lvgl/src/drivers/sdl/lv_sdl_mouse.h:31] `lv_indev_t * lv_sdl_mouse_create(void);` — move both calls into `board/native_sdl/board.c`, they must not remain in `main.c`. |
| LVGL `lv_bar` widget | bundled | Splash loading bar | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/widgets/bar/lv_bar.h:67,79] `lv_obj_t * lv_bar_create(lv_obj_t * parent);` / `void lv_bar_set_value(lv_obj_t * obj, int32_t value, lv_anim_enable_t anim);` |
| LVGL `lv_obj_fade_in`/`lv_obj_fade_out` | bundled | Splash logo/name fade-in without hand-rolling an `lv_anim_t` for opacity | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/core/lv_obj_style.h:254,262] `void lv_obj_fade_in(lv_obj_t * obj, uint32_t time, uint32_t delay);` / `void lv_obj_fade_out(lv_obj_t * obj, uint32_t time, uint32_t delay);` — simpler and less error-prone than manually configuring `lv_anim_init`/`lv_anim_set_exec_cb` for a plain fade, satisfies D-01 directly. |
| LVGL `lv_timer` | bundled | Drives the splash's staged storage-init sequence without blocking | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/misc/lv_timer.h:91,97] `lv_timer_t * lv_timer_create(lv_timer_cb_t timer_xcb, uint32_t period, void * user_data);` / `void lv_timer_delete(lv_timer_t * timer);` |
| C standard library (`stdio.h`, `dirent.h` on POSIX/Linux) | system | Native storage backend reads fixture files from `assets/mock/` | Plain `fopen`/`fread`/`opendir`/`readdir` — no extra dependency. `dirent.h` is POSIX; this dev machine is Linux so it's available, but note the portability caveat below (see Pitfalls). |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Hand-rolled `lv_anim_t` opacity animation for splash fade-in | `lv_obj_fade_in()` helper | The helper is a documented, tested convenience wrapper over the same `lv_anim` machinery — no reason to hand-roll it for a plain fade; fall back to raw `lv_anim` only if a non-linear/custom path is later needed. |
| `lv_timer`-staged storage init driving the splash bar | A single blocking `ratimos_storage_init()` call + a fake `lv_anim` timer for the bar | Rejected — this is explicitly what D-03 forbids ("not a fake timer"); staged real steps is the only way to satisfy both D-02 (~2s) and D-03 (real feedback) simultaneously. |
| Fixed-size caller-provided arrays for `ratimos_storage_list_*()` | `malloc`-backed dynamic lists | Fixture counts are 3-4 items (D-13); avoiding heap allocation in the Storage API now avoids an ESP32 heap-fragmentation habit that would need unlearning once this code runs on real hardware with tighter RAM. |

**Installation:** No new dependencies. `lib_deps = lvgl/lvgl@^9.2.2` in `platformio.ini` is already sufficient — LVGL's `lv_bar`, `lv_anim`, `lv_timer`, and SDL drivers are all part of the core LVGL package already vendored, not separate libraries.

**Version verification:** LVGL version confirmed by reading the vendored `lv_version.h` directly (see table above) — this is stronger than a registry lookup since it reflects exactly what this project's build will compile against. No `npm view`/`pip index`/`cargo search` applicable — this is a PlatformIO/C project with no new package additions this phase.

## Package Legitimacy Audit

No external packages are being added in this phase. `platformio.ini`'s existing `lib_deps` (`lvgl/lvgl@^9.2.2`) was already present in the Phase 0 baseline and is unchanged by this phase's scope. The Package Legitimacy Gate is not applicable — skip.

**Packages removed due to [SLOP] verdict:** none (no packages evaluated)
**Packages flagged as suspicious [SUS]:** none (no packages evaluated)

## Architecture Patterns

### System Architecture Diagram

```
┌─────────────────────────── main.c (generic, board-agnostic) ───────────────────────────┐
│  lv_init()                                                                              │
│    → board_display_init()   ─┐                                                          │
│    → board_input_init()      │  ONLY these 3 calls + lv_timer_handler() +               │
│    → ratimos_splash_show()   │  board_tick() may appear in main.c.                      │
│    → loop { lv_timer_handler(); board_tick(idle); }                                     │
└──────────────┬─────────────────────────────────────────────────┬────────────────────────┘
               │                                                  │
   ┌───────────▼────────────┐                        ┌────────────▼─────────────┐
   │  src/board/native_sdl/  │  (compiled this phase)  │ src/board/waveshare_s3_35/│ (stub only,
   │  board.c/.h             │                        │  board.c/.h                │  excluded from
   │  - lv_sdl_window_create │                        │  - TODO bodies, no SDK     │  native_sim build
   │  - lv_sdl_mouse_create  │                        │    includes (must compile │  via build_src_filter,
   │  - lv_tick_set_cb(SDL_  │                        │    with plain gcc, no IDF) │  syntax-checked
   │    GetTicks)            │                        │                            │  separately)
   │  - board_tick(): SDL_   │                        │                            │
   │    Delay(ms)            │                        │                            │
   └────────────┬────────────┘                        └────────────────────────────┘
                │
   ┌────────────▼──────────────────────── Splash screen (ratimos/splash.c) ─────────────┐
   │  lv_obj_fade_in(logo, 2000, 0)                                                      │
   │  lv_timer_create(step_cb, ~350ms, NULL)  → each tick calls ONE real storage-init    │
   │    step (mount → index photos → tracks → letters → games → settings) and calls     │
   │    lv_bar_set_value(bar, step/total*100, LV_ANIM_ON)                                │
   │  on last step: lv_timer_delete(self); ratimos_home_screen_show(NULL);               │
   └────────────┬─────────────────────────────────────────────────────────────────────────┘
                │  (touch during splash: no input handlers registered — D-04, tap-to-skip
                │   is satisfied by simply not wiring a click callback on the splash screen)
                ▼
   ┌─────────────────────────────── src/ratimos/ (existing shell) ──────────────────────┐
   │  home_screen.c → 5 tiles → ratimos_{jogos,musica,album,cartas,config}_show()        │
   │  each app: ratimos_app_shell_create() → populate content via Storage API calls only │
   └────────────┬─────────────────────────────────────────────────────────────────────────┘
                │  ratimos_storage_list_photos()/list_tracks()/list_letters()/list_games()/
                │  get_settings()  — synchronous, fixed-size output buffers
                ▼
   ┌─────────────────────────────────── src/storage/ ───────────────────────────────────┐
   │  content_api.h (public, backend-agnostic signatures)                                │
   │  photos.c / tracks.c / letters.c / games.c / settings.c                             │
   │    → each fopen()s files under assets/mock/<domain>/ at call time (or once, cached   │
   │      after the splash's "index" step — see Pitfall 1)                                │
   └──────────────────────────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure
```
src/
├── main.c                        # generic entry point — zero SDL2/board-specific code
├── board/
│   ├── board.h                   # shared interface: board_display_init/input_init/tick declarations
│   ├── native_sdl/
│   │   ├── board.c               # implements the interface using SDL2 + LVGL SDL drivers
│   │   └── board.h               # (optional, if board.h needs a per-board split — see below)
│   └── waveshare_s3_35/
│       └── board.c               # compiling stub: TODO bodies, NO esp-idf/Arduino includes
├── storage/
│   ├── content_api.h             # public, backend-agnostic function signatures + domain structs
│   ├── photos.c
│   ├── tracks.c
│   ├── letters.c
│   ├── games.c
│   └── settings.c
└── ratimos/                       # existing shell — unchanged in spirit, apps now call storage/
    ├── splash.c / splash.h        # new: boot identity screen
    ├── theme.h / theme.c          # existing, reused
    ├── app_shell.c / .h           # existing, reused
    ├── home_screen.c / .h         # existing, reused
    ├── row_list.c / .h            # existing, reused
    ├── status_bar.c / .h          # existing, reused
    └── apps/*.c                   # existing files, content source swapped to storage/ calls

assets/
└── mock/                          # NEW — fixture files, native-only, never in esp32s3 builds
    ├── photos/                    # 3-4 items (see Pitfall 2 on file extension choice)
    ├── tracks/
    ├── letters/
    └── games/
```

### Pattern 1: Generic `main.c` + Board Interface Struct/Functions

**What:** `main.c` calls exactly three board functions (`board_display_init()`, `board_input_init()`, `board_tick()`) plus LVGL's own `lv_init()`/`lv_timer_handler()`. All platform headers (`<SDL2/SDL.h>`, future `esp_lcd.h`) are `#include`d only inside `board/<variant>/board.c`.
**When to use:** Any code that currently does `#include <SDL2/SDL.h>` in `main.c` (today: `lv_sdl_window_create`, `lv_sdl_mouse_create`, `lv_tick_set_cb(SDL_GetTicks)`, `SDL_Delay`) must move.
**Example (native_sdl/board.c):**
```c
// Source: adapted from src/main.c (existing repo code), split per D-05/D-06/D-08
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "../board.h"
#include "../../ratimos/theme.h"

void board_display_init(void)
{
    lv_display_t * disp = lv_sdl_window_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    (void) disp;
    lv_tick_set_cb(SDL_GetTicks);   /* SDL-specific tick source lives HERE, not in main.c */
}

void board_input_init(void)
{
    lv_indev_t * mouse = lv_sdl_mouse_create();
    (void) mouse;
}

void board_tick(uint32_t idle_time_ms)
{
    SDL_Delay(idle_time_ms > 0 ? idle_time_ms : 5);
}
```
```c
// Source: adapted from src/main.c (existing repo code)
#include "lvgl.h"
#include "board/board.h"
#include "ratimos/splash.h"

int main(void)
{
    lv_init();
    board_display_init();
    board_input_init();

    ratimos_splash_show();   /* replaces the current direct ratimos_home_screen_show(NULL) call */

    while (1) {
        uint32_t idle = lv_timer_handler();
        board_tick(idle);
    }
    return 0;
}
```

### Pattern 2: Splash-Driven Staged Storage Init (resolves D-02 vs D-03 tension)

**What:** Instead of one blocking `ratimos_storage_init()` call plus a cosmetic timer for the loading bar, break storage init into named discrete steps and drive them one-per-tick from an `lv_timer` owned by the splash screen. Each tick performs real work (mounting the native backend, indexing one content domain) and immediately reflects it in the bar's value — this is "real progress," not decoration, while the timer's period gives you control over total splash duration (~2s ÷ number of steps).
**When to use:** Any boot sequence that must show genuine progress but where the real work completes too fast (sub-millisecond fixture reads on a dev PC) to naturally fill a human-perceptible ~2s window.
**Example:**
```c
// Source: pattern derived from lv_timer.h (verified) + D-02/D-03 requirements
typedef enum {
    SPLASH_STEP_MOUNT, SPLASH_STEP_PHOTOS, SPLASH_STEP_TRACKS,
    SPLASH_STEP_LETTERS, SPLASH_STEP_GAMES, SPLASH_STEP_SETTINGS,
    SPLASH_STEP_DONE
} splash_step_t;

static lv_obj_t * s_bar;
static splash_step_t s_step = SPLASH_STEP_MOUNT;

static void splash_step_cb(lv_timer_t * t)
{
    switch (s_step) {
        case SPLASH_STEP_MOUNT:    ratimos_storage_mount();          break;
        case SPLASH_STEP_PHOTOS:   ratimos_storage_index_photos();   break;
        case SPLASH_STEP_TRACKS:   ratimos_storage_index_tracks();   break;
        case SPLASH_STEP_LETTERS:  ratimos_storage_index_letters();  break;
        case SPLASH_STEP_GAMES:    ratimos_storage_index_games();    break;
        case SPLASH_STEP_SETTINGS: ratimos_storage_index_settings(); break;
        default: break;
    }
    lv_bar_set_value(s_bar, (s_step + 1) * 100 / SPLASH_STEP_DONE, LV_ANIM_ON);
    s_step++;
    if (s_step >= SPLASH_STEP_DONE) {
        lv_timer_delete(t);
        ratimos_home_screen_show(NULL);
    }
}
```
6 steps × ~330ms period ≈ 2s total, and every step is real indexing work, not a sleep.

### Anti-Patterns to Avoid
- **SDL2 calls anywhere outside `board/native_sdl/`:** the entire point of this phase's HAL split. Grep for `SDL_` or `#include <SDL2` outside `board/native_sdl/` as a mechanical verification check.
- **A storage backend interface/vtable for a single implementation:** `board/` needs two concrete implementations right now (native_sdl + waveshare stub), so its interface split earns its complexity. `storage/` has exactly one real implementation this phase (native/mock) — do not build a `storage_backend_t` function-pointer struct or `#ifdef`-based backend selector yet; that's the same premature-abstraction mistake D-10 already warns against for async, just for a different axis. Add the seam in Phase 5 when the SD backend is actually being built.
- **`malloc`/dynamic-list APIs for 3-4 item lists:** fixed-size caller buffers (`size_t ratimos_storage_list_photos(ratimos_photo_t * out, size_t max_count)`) are simpler, match the tiny fixture counts (D-13), and avoid an ESP32 heap habit that will need to be relearned later.
- **`board_waveshare_s3_35` stub including any ESP-IDF/Arduino header:** No `esp32s3` PlatformIO environment/toolchain exists yet this phase (D-07 explicitly defers it). If the stub includes e.g. `esp_lcd_panel_ops.h`, it cannot be compiled or even syntax-checked without the ESP-IDF toolchain installed, defeating "the stub just needs to compile as a translation unit to prove the interface holds." Keep the stub's `.c` file pure C89/C11 with TODO-only bodies (e.g. `return false;` or empty) so it compiles with plain `gcc`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Splash fade-in | Custom `lv_anim_t` opacity animation | `lv_obj_fade_in(obj, time, delay)` | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/core/lv_obj_style.h:254] — one call does what D-01 asks for. |
| Loading-bar UI widget | Custom drawn rectangle + manual width calc | LVGL `lv_bar` widget (`lv_bar_create`/`lv_bar_set_value`) | [VERIFIED: .pio/libdeps/native_sim/lvgl/src/widgets/bar/lv_bar.h:67,79] — built-in widget with animated value transitions (`LV_ANIM_ON`) already handles the visual interpolation between steps. |
| Storage backend switching (native vs future SD) | A `storage_backend_t` interface/registry now | Direct concrete functions now; introduce the seam in Phase 5 | Only one backend exists this phase — see Anti-Pattern above. |

**Key insight:** This phase's entire job is separating three already-good-but-tangled concerns (board, storage, UI). The temptation is to over-build the seams (interfaces, registries, async plumbing) in anticipation of future phases. Resist it everywhere except `board/`, which is the one seam this phase's own success criteria explicitly require to have two concrete implementations today.

## Runtime State Inventory

> Omitted — this is a greenfield addition (new `board/`, `storage/`, `assets/mock/` layers) on top of an existing local-only simulator codebase. No rename/refactor of existing identifiers, no external services, no OS-registered state, no secrets. The only "migration" is moving existing SDL2 calls out of `src/main.c` into `src/board/native_sdl/`, which is a straightforward code move covered in Architecture Patterns above, not a runtime-state concern.

## Common Pitfalls

### Pitfall 1: Re-reading fixture files from disk on every app screen open
**What goes wrong:** If `ratimos_storage_list_photos()` does `fopen`/`fread` fresh every time the album app is opened (rather than caching the result from the splash's "index" step), navigating home→album→home→album repeatedly does unnecessary disk I/O and risks subtle bugs if a fixture file is malformed (crash triggered lazily on nth navigation instead of at boot).
**Why it happens:** The most naive implementation of "read through the Storage API" is "have the getter function do the file I/O inline."
**How to avoid:** Have the splash's `ratimos_storage_index_*()` steps actually populate an in-memory static array (module-level `static ratimos_photo_t s_photos[4];` + a count), and have `ratimos_storage_list_photos()` just copy from that array. This also makes the splash's progress bar meaningful — "indexing" is real work (open+parse files once), not a no-op placeholder for a getter that does all the work later.
**Warning signs:** App screens feel slow to open on first touch; a malformed fixture file crashes navigation instead of crashing at boot where it's easier to diagnose.

### Pitfall 2: Fixture files under `assets/mock/` silently excluded from git by the existing `.gitignore`
**What goes wrong:** [VERIFIED: .gitignore:1-4] the project's `.gitignore` contains a bare `*.bin` rule. If any fixture (especially a "photo" stored as a raw pixel dump, which is a natural choice for a decoder-free placeholder image) is named e.g. `photo1.bin`, it will be silently git-ignored and the fixture will exist locally but never actually land in the repo — directly breaking D-14's intent ("these fixtures live in the git repository").
**Why it happens:** `.gitignore`'s `*.bin` rule was written for build artifacts (`.pio/build/**/*.bin` firmware images), not anticipated to collide with data fixtures.
**How to avoid:** Do not use the `.bin` extension for any file under `assets/mock/`. Use `.txt`/`.json`/`.dat`/`.ppm` or similar instead, OR add an explicit negation rule (`!assets/mock/**/*.bin`) to `.gitignore` if a binary format is truly needed. Verify with `git status assets/mock/` after creating fixtures, before considering the task done.
**Warning signs:** `git status` shows fewer files under `assets/mock/` than expected; `git ls-files assets/mock/` is missing entries present on disk.

### Pitfall 3: Photo fixtures implying real JPEG/PNG decode is in scope for Phase 1
**What goes wrong:** D-14 mentions "stock-style placeholder images" for photo fixtures, which could be read as "the album app must decode and paint real image pixels this phase." But no JPEG/PNG decoder library is part of this phase's stack — `JPEGDEC`/`TJpg_Decoder` are explicitly tied to camera/SD bring-up (traceability: ALBUM-01 → Phase 5), and pulling one in now to satisfy a Phase 1 fixture would violate the "don't build for the future prematurely" principle already established for storage/async.
**Why it happens:** "Placeholder image" is ambiguous between "a file that represents a photo" (metadata-only, fine for Phase 1) and "a file that renders as an actual image on screen" (needs a decoder, Phase 5's job).
**How to avoid:** Treat photo fixtures as metadata + a reference filename in Phase 1 (title, id, path string) and keep rendering the existing placeholder tile (`ratimos_panel_create` + label, as `album_app.c` already does) but now populated by the Storage API's returned title/count instead of a hardcoded loop count of 4. Confirm this interpretation with the user/planner if the "stock-style placeholder image" language was meant more literally — flagged below as an Open Question.
**Warning signs:** Scope creep into adding an image-decode dependency this phase; a Phase 1 plan task titled anything like "decode and render placeholder JPEGs."

### Pitfall 4: `dirent.h`/POSIX directory listing assumed portable
**What goes wrong:** If the native storage backend uses `opendir`/`readdir` to discover fixture files dynamically (rather than a hardcoded manifest), this only compiles on POSIX hosts (Linux/macOS). PlatformIO's `platform = native` build can also target Windows hosts, where `dirent.h` isn't in the standard MSVC toolchain (though MinGW provides it).
**Why it happens:** The dev machine here is Linux, so `dirent.h`-based code compiles fine locally and the portability gap goes unnoticed until someone builds on Windows.
**How to avoid:** Either (a) accept the POSIX-only constraint explicitly for now since this project's only build machine is Linux (document it in a comment), or (b) avoid directory scanning entirely — use a small hardcoded manifest (an array of filenames per domain) in `storage/photos.c` etc. and `fopen()` each by name. Given fixture counts are only 3-4 items per domain (D-13), a hardcoded manifest is simpler and sidesteps the portability question entirely — recommended.
**Warning signs:** Build failure on a non-Linux host; `dirent.h: No such file or directory`.

### Pitfall 5: `board_waveshare_s3_35` stub accidentally gets linked into the `native_sim` build
**What goes wrong:** If `build_src_filter` isn't updated to exclude `board/waveshare_s3_35/*`, PlatformIO will try to compile AND link both `board/native_sdl/board.c` and `board/waveshare_s3_35/board.c` into the same `native_sim` binary — both define `board_display_init()` etc., causing a duplicate-symbol linker error.
**Why it happens:** The existing `build_src_filter` only excludes `-<esp32/*>` [VERIFIED: platformio.ini:21-23], a pattern from the Phase 0 baseline that doesn't yet know about the new `board/` split.
**How to avoid:** Update `build_src_filter` to `+<*>` / `-<board/waveshare_s3_35/*>` (dropping the now-vacuous `-<esp32/*>` line, or keeping it harmlessly since no `esp32/` directory exists). Verify with `pio run -e native_sim` completing without duplicate-symbol errors, and separately syntax-check the stub with a standalone `gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c` (or equivalent) to prove D-07's "compiles as a translation unit" without adding a real `esp32s3` PlatformIO environment.
**Warning signs:** Linker errors mentioning `board_display_init` defined twice.

## Code Examples

### `board.h` — shared interface contract (D-05)
```c
// Source: derived directly from CONTEXT.md D-05 (locked decision)
#ifndef RATIMOS_BOARD_H
#define RATIMOS_BOARD_H
#include <stdint.h>

void board_display_init(void);
void board_input_init(void);
void board_tick(uint32_t idle_time_ms);

#endif
```

### `platformio.ini` — extending `build_src_filter` for the board split
```ini
; Source: adapted from platformio.ini (existing repo code), extended per D-06/D-07
build_src_filter =
    +<*>
    -<board/waveshare_s3_35/*>
```

### Storage API domain struct + list function shape (allocation-free)
```c
// Source: pattern derived from D-09/D-10/D-13 (locked decisions) — no verified upstream example,
// this is a project-specific convention recommendation, not a library API.
typedef struct {
    char id[16];
    char title[64];
} ratimos_letter_t;

/* Returns the number of items written into `out` (<= max_count). Synchronous, no allocation. */
size_t ratimos_storage_list_letters(ratimos_letter_t * out, size_t max_count);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|---------------|--------|
| `lv_scr_load()` / pre-LVGL-9 screen API | `lv_screen_load()` | LVGL v9 API rename | [VERIFIED: src/ratimos/app_shell.c:29, src/ratimos/home_screen.c:80] existing code already correctly uses `lv_screen_load` — no migration needed, just confirming the codebase is already on the current API, avoid "fixing" it to the older `lv_scr_load` name if referencing older tutorials. |

**Deprecated/outdated:** None encountered specific to this phase's scope — LVGL 9.5 is current, no API in use here (`lv_obj_create`, `lv_bar`, `lv_anim`, `lv_timer`, SDL drivers) is flagged deprecated in the vendored source.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Fixture manifests should be hardcoded filename arrays rather than directory-scanned, for portability | Pitfall 4 | Low — if the planner instead chooses `dirent.h` scanning, it still works fine on this project's actual (Linux) dev machine; only matters if a Windows build host is ever used. |
| A2 | "Stock-style placeholder images" (D-14) means metadata/placeholder-tile fixtures, not real decoded pixel rendering | Pitfall 3 | Medium — if the user actually wants real (if generic) images rendering on the album screen in Phase 1, the plan would need to add a minimal raw-pixel-format loader (not a full JPEG decoder) to avoid pulling in Phase 5's JPEGDEC dependency early. Flagged as an Open Question below. |
| A3 | `board_tick(uint32_t idle_time_ms)` is the right signature (idle time passed in from `lv_timer_handler()`'s return value) | Pattern 1 | Low — alternative signatures (no-arg `board_tick(void)` with SDL_Delay hardcoded inside, idle time read via a separate getter) are functionally equivalent; this is an implementation-detail choice within Claude's discretion per CONTEXT.md, not a locked decision. |
| A4 | Splash timer step period (~330ms × 6 steps ≈ 2s) is an acceptable way to hit "~2s" (D-02) | Pattern 2 | Low — D-02 says "~2s," not an exact figure; any reasonable per-step pacing (250-400ms) satisfies it. Exact number of storage-init steps (5 vs 6) is also a reasonable implementation choice. |

## Open Questions (RESOLVED)

1. **Does "stock-style placeholder images" (D-14) require actual pixel rendering, or is a placeholder tile + title metadata sufficient for Phase 1?**
   - What we know: ALBUM-01 (real thumbnail grid + full-screen view) is traced to Phase 5, not Phase 1; no JPEG/PNG decoder is part of this phase's stack per CLAUDE.md's own sequencing (JPEGDEC tied to camera/SD bring-up).
   - What's unclear: Whether the user's mental model for "success" on this phase includes seeing generic stock photos actually rendered in the album grid vs. just labeled placeholder tiles proving the Storage API wiring.
   - Recommendation: Default to metadata-only fixtures + the existing placeholder-tile pattern (already in `album_app.c`) fed by real counts/titles from the Storage API. If the user pushes back during `/gsd-verify-work`, adding a minimal raw-pixel-format (not JPEG) loader is a small follow-up, not a re-plan.
   - **Resolved:** 01-03-PLAN.md Task 1 settled this per the recommendation — `photos.c` is metadata-only (title/id read from `assets/mock/photos/*.txt`'s first line), rendered through the existing placeholder-tile pattern in `album_app.c` (Task 2), with no image-decode dependency added.

2. **Should `board_waveshare_s3_35`'s "compiles as a translation unit" requirement (D-07) be verified via an ad-hoc `gcc -fsyntax-only` step, or is there a preferred PlatformIO-native way to syntax-check a file that isn't part of any active build environment?**
   - What we know: No `esp32s3` PlatformIO environment exists yet (explicitly deferred); `native_sim`'s `build_src_filter` must exclude the stub to avoid duplicate-symbol linking.
   - What's unclear: Whether the planner wants this as a manual one-off verification command (documented in the plan) or a scripted check (e.g., a Makefile target or CI step) — the latter is more durable but may be over-engineering for a single-developer, no-CI project at this stage.
   - Recommendation: A manual `gcc -fsyntax-only` command in the plan's verification steps is sufficient for Phase 1; revisit if/when CI is ever introduced.
   - **Resolved:** 01-01-PLAN.md Task 2's `<verify>` settled this per the recommendation — a manual `gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c` command runs alongside `pio test`/`pio run` in that task's automated verify step; no Makefile/CI target was added.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| PlatformIO Core (`pio`) | Building/running `native_sim` | [VERIFIED: local `pio --version`] ✓ | 6.1.19 | — |
| LVGL (vendored via `lib_deps`) | UI toolkit | [VERIFIED: .pio/libdeps/native_sim/lvgl/lv_version.h] ✓ | 9.5.0 | — |
| SDL2 (system library) | native_sim window/mouse driver | ✓ (existing `native_sim` build already links `-lSDL2` and presumably has built before) | system-provided | — |
| GCC/host C toolchain | `platform = native` builds + `gcc -fsyntax-only` stub check | Not independently re-verified this session (assume present since `pio` already built `native_sim` previously — `.pio/build` exists) | — | — |

**Missing dependencies with no fallback:** none identified.
**Missing dependencies with fallback:** none identified.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | PlatformIO's built-in Unity test runner for `platform = native` [CITED: docs.platformio.org/en/latest/advanced/unit-testing/index.html] |
| Config file | none yet — no `test/` directory exists in this repo currently [VERIFIED: repo file listing shows no `test/` directory] |
| Quick run command | `pio test -e native_sim -f test_storage` (single test suite) |
| Full suite command | `pio test -e native_sim` |
| File Exists? | ❌ — Wave 0 gap, see below |

**Mechanics (CITED, not yet exercised in this repo):** PlatformIO's unit-testing engine treats any `test/test_*/` folder as an independent test binary with its own `main()`; for `platform = native`, PlatformIO requires a host GCC toolchain (already present, since `native_sim` already builds) but installs no toolchain itself. `#include <unity.h>` + `UNITY_BEGIN()`/`UNITY_END()` is the minimal skeleton.

### Phase Requirement → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|---------------------|-------------|
| SHELL-01 | Home → 5 sections → back navigation | manual/smoke (LVGL screen transitions have no headless test harness in this codebase) | `pio run -e native_sim && .pio/build/native_sim/program` (manual click-through) | ❌ Wave 0 (no change — this stays manual-only; justification: no LVGL headless/mock-display test infra exists or is proposed for this phase) |
| SHELL-03 | Splash shows before home screen, real progress, no skip | manual/smoke (visual timing + fade cannot be meaningfully unit-tested without a virtual display) | same manual run as above, observe splash duration/behavior | ❌ Wave 0 (manual-only, same justification) |
| (supporting) Storage API returns correct counts/titles per domain from `assets/mock/` fixtures | unit | `pio test -e native_sim -f test_storage` | ❌ Wave 0 — needs `test/test_storage/test_storage.c` |
| (supporting) `board_waveshare_s3_35` stub compiles standalone | build/syntax-check | `gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c` | n/a — ad-hoc command, no file needed |

### Sampling Rate
- **Per task commit:** `pio run -e native_sim` (build must stay green) + `pio test -e native_sim -f test_storage` once that suite exists
- **Per wave merge:** `pio test -e native_sim` (full suite) + manual click-through of all 5 sections + splash observation
- **Phase gate:** Full suite green, manual UAT script (splash timing/no-skip, 5-section round-trip, storage-backed content visible in each app) before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `test/test_storage/test_storage.c` — covers the Storage API's list/get functions against `assets/mock/` fixtures (pure C, no LVGL dependency, genuinely unit-testable)
- [ ] No `unity_config.h` customization needed — PlatformIO's default Unity config is sufficient for this simple case
- [ ] Framework install: none — PlatformIO's native unit-testing support requires no additional `lib_deps` beyond an existing host GCC toolchain, which this project's `native_sim` build already depends on

*(UI/navigation/splash behavior stays manual-only for this phase — there is no LVGL headless-display test harness in this codebase, and building one is out of scope for a shell/storage phase. Revisit if a future phase's UI complexity justifies the investment.)*

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-------------------|
| V2 Authentication | no | No auth surface in this phase (local simulator only, no network) |
| V3 Session Management | no | N/A |
| V4 Access Control | no | Single-user local device, no access boundaries this phase |
| V5 Input Validation | no | No untrusted external input this phase (fixture files are developer-authored, in-repo) |
| V6 Cryptography | no | N/A |
| V12 File & Resources | yes | Storage API must build fixture file paths from a fixed, hardcoded manifest (see Pitfall 4) rather than concatenating any external/user-controlled string — establishes the safe pattern now since Phase 5 reuses this same code path against real SD content where a malformed filename from sync could otherwise become a path-traversal vector. |

### Known Threat Patterns for this stack
| Pattern | STRIDE | Standard Mitigation |
|---------|--------|----------------------|
| Path traversal via constructed fixture/content filenames | Tampering / Information Disclosure | Use a fixed, compiled-in manifest of known filenames per content domain (not directory-scan-and-trust or string-concatenation from any dynamic source) — even though this phase's fixtures are 100% developer-controlled, establishing this discipline now means Phase 5's real SD-backed storage and Phase 7's cloud-synced filenames inherit a safe pattern instead of needing a retrofit. |

## Sources

### Primary (HIGH confidence)
- `.pio/libdeps/native_sim/lvgl/lv_version.h` — LVGL 9.5.0 confirmed by reading the vendored source in this repo
- `.pio/libdeps/native_sim/lvgl/src/widgets/bar/lv_bar.h` — `lv_bar_create`/`lv_bar_set_value` signatures
- `.pio/libdeps/native_sim/lvgl/src/misc/lv_anim.h` — `lv_anim_*` API surface
- `.pio/libdeps/native_sim/lvgl/src/misc/lv_timer.h` — `lv_timer_create`/`lv_timer_delete` signatures
- `.pio/libdeps/native_sim/lvgl/src/core/lv_obj_style.h` — `lv_obj_fade_in`/`lv_obj_fade_out` signatures
- `.pio/libdeps/native_sim/lvgl/src/drivers/sdl/lv_sdl_window.h`, `lv_sdl_mouse.h` — SDL driver signatures
- Existing repo files read directly this session: `src/main.c`, `platformio.ini`, `lv_conf.h`, `.gitignore`, all of `src/ratimos/*` and `src/ratimos/apps/*`

### Secondary (MEDIUM confidence)
- `.planning/research/ARCHITECTURE.md` (this project's own prior research pass) — board/storage/sync layering rationale, anti-patterns for apps touching storage directly
- `.planning/research/STACK.md` (this project's own prior research pass) — confirms LVGL 9.5.x, no new-package implications for this phase

### Tertiary (LOW confidence, cross-checked)
- [PlatformIO Unit Testing docs](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) — WebSearch-sourced summary of native-platform Unity testing mechanics, cross-referenced against PlatformIO's own official docs domain (docs.platformio.org), not independently fetched/read in full this session — treat exact CLI flag behavior as needing a quick sanity check (`pio test --help`) when actually writing the Wave 0 test scaffold.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — LVGL APIs verified directly from vendored source in this repo, no external claims needed
- Architecture: MEDIUM — board/storage split rationale is sound and directly derived from locked CONTEXT.md decisions plus this project's own prior ARCHITECTURE.md research, but the exact "staged splash timer" pattern is this session's own synthesis, not a documented external pattern
- Pitfalls: MEDIUM-HIGH — Pitfalls 2 and 5 are verified directly against this repo's actual `.gitignore`/`platformio.ini` content; Pitfalls 1, 3, 4 are reasoned from the locked decisions and general embedded-C practice

**Research date:** 2026-08-26
**Valid until:** 30 days (stable domain — no fast-moving external dependencies added this phase; re-verify LVGL version if `platformio.ini`'s `lib_deps` pin changes)

