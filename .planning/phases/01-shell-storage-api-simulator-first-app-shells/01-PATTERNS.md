# Phase 1: Shell, Storage API & Simulator-First App Shells - Pattern Map

**Mapped:** 2026-08-27
**Files analyzed:** 20 (new/modified)
**Analogs found:** 18 / 20

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|--------------------|------|-----------|-----------------|----------------|
| `src/board/board.h` | interface/config | request-response | `src/ratimos/app_shell.h` (struct+function contract style) | role-match |
| `src/board/native_sdl/board.c` | driver/HAL | event-driven | `src/main.c` (existing SDL2 init calls, to be extracted verbatim) | exact |
| `src/board/waveshare_s3_35/board.c` | driver/HAL stub | event-driven | `src/board/native_sdl/board.c` (same interface, TODO bodies) | role-match (sibling, no existing analog) |
| `src/main.c` (rewritten) | controller/entry-point | event-driven | current `src/main.c` (existing) | exact (self-refactor) |
| `platformio.ini` (edited) | config | build | existing `[env:native_sim]` `build_src_filter` | exact |
| `src/storage/content_api.h` | model/interface | CRUD | `src/ratimos/theme.h` (header declaring structs + free functions, no state) | role-match |
| `src/storage/photos.c` | service | CRUD (file-I/O + in-memory cache) | `src/ratimos/apps/album_app.c` (data shape consumed) + Pitfall-1 caching pattern (RESEARCH.md) | partial (no existing storage layer; new pattern from RESEARCH.md) |
| `src/storage/tracks.c` | service | CRUD (file-I/O + in-memory cache) | same as photos.c | partial |
| `src/storage/letters.c` | service | CRUD (file-I/O + in-memory cache) | same as photos.c | partial |
| `src/storage/games.c` | service | CRUD (file-I/O + in-memory cache) | same as photos.c | partial |
| `src/storage/settings.c` | service | CRUD (file-I/O + in-memory cache) | same as photos.c | partial |
| `src/ratimos/splash.c` / `.h` | component/screen | event-driven (staged init via `lv_timer`) | `src/ratimos/home_screen.c` (screen-build + `lv_screen_load` pattern) | role-match |
| `src/ratimos/home_screen.c` (unchanged in spirit) | component/screen | request-response | itself (no change needed) | exact |
| `src/ratimos/app_shell.c` (unchanged) | component/layout | request-response | itself | exact |
| `src/ratimos/row_list.c` (unchanged, only long-mode tweak) | component | request-response | itself | exact |
| `src/ratimos/apps/album_app.c` (modified) | component/screen | CRUD (read via Storage API) | `src/ratimos/apps/cartas_app.c` (empty-state row pattern) + itself for tile grid | exact (self + sibling) |
| `src/ratimos/apps/cartas_app.c` (modified) | component/screen | CRUD (read via Storage API) | itself (existing empty-state row already matches target shape) | exact |
| `src/ratimos/apps/musica_app.c` (modified) | component/screen | CRUD (read via Storage API) | itself + `cartas_app.c` (empty-state) | exact |
| `src/ratimos/apps/jogos_app.c` (modified) | component/screen | CRUD (read via Storage API) | itself (already lists static rows, swap source) | exact |
| `src/ratimos/apps/config_app.c` (modified) | component/screen | CRUD (read via Storage API for settings) | itself (already lists static rows, swap source) | exact |
| `test/test_storage/test_storage.c` (new) | test | CRUD verification | none (no `test/` dir exists yet) | no analog — use PlatformIO Unity skeleton from RESEARCH.md |
| `assets/mock/**/*` (fixture files + manifest) | config/data | file-I/O | none (new asset tree) | no analog |

## Pattern Assignments

### `src/board/board.h` (interface, request-response)

**Analog:** `src/ratimos/app_shell.h` — shows this project's convention for a small header declaring a plain C struct + free functions, no OOP/vtable wrapping.

**Header shape convention** (`src/ratimos/app_shell.h:1-19`):
```c
#ifndef RATIMOS_APP_SHELL_H
#define RATIMOS_APP_SHELL_H

#include "lvgl.h"

typedef struct {
    lv_obj_t * screen;
    lv_obj_t * content;
} ratimos_app_shell_t;

ratimos_app_shell_t ratimos_app_shell_create(const char * section_label,
                                              const char * bottom_right_hint);

#endif
```
Apply the same include-guard + doc-comment-above-declaration style to `board.h`. RESEARCH.md already gives the exact target shape (D-05 locked):
```c
#ifndef RATIMOS_BOARD_H
#define RATIMOS_BOARD_H
#include <stdint.h>

void board_display_init(void);
void board_input_init(void);
void board_tick(uint32_t idle_time_ms);

#endif
```

---

### `src/board/native_sdl/board.c` (driver, event-driven)

**Analog:** current `src/main.c` (lines 1-28) — every SDL2/LVGL call here must move verbatim into this file.

**Imports pattern** (`src/main.c:1-8`):
```c
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ratimos/theme.h"
#include "ratimos/home_screen.h"
```

**Core pattern to extract** (`src/main.c:12-18, 22-25`):
```c
lv_tick_set_cb(SDL_GetTicks);
lv_display_t * disp = lv_sdl_window_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
lv_indev_t * mouse = lv_sdl_mouse_create();
(void) disp;
(void) mouse;
...
uint32_t idle = lv_timer_handler();
SDL_Delay(idle > 0 ? idle : 5);
```
Split as: `board_display_init()` owns `lv_sdl_window_create` + `lv_tick_set_cb`; `board_input_init()` owns `lv_sdl_mouse_create`; `board_tick(idle_time_ms)` owns `SDL_Delay`. Include `../board.h` and `../../ratimos/theme.h` (for `RATIMOS_SCREEN_W/H`).

**Error handling:** none exists in current code (no init failure checks) — preserve that (Phase 1 doesn't add error handling here per RESEARCH.md scope).

---

### `src/board/waveshare_s3_35/board.c` (driver stub, event-driven)

**Analog:** sibling `src/board/native_sdl/board.c` for signature shape only — body must NOT copy any SDL2/LVGL calls, must be pure TODO/stub per D-07 and RESEARCH Anti-Patterns (no ESP-IDF/Arduino includes, must compile with plain gcc).

```c
#include "../board.h"

void board_display_init(void) { /* TODO Phase 3: ST7796 init */ }
void board_input_init(void)   { /* TODO Phase 3: FT6336 init */ }
void board_tick(uint32_t idle_time_ms) { (void) idle_time_ms; /* TODO Phase 3 */ }
```

---

### `src/main.c` (rewritten, event-driven)

**Analog:** itself — refactor in place, following RESEARCH.md Pattern 1 exactly:
```c
#include "lvgl.h"
#include "board/board.h"
#include "ratimos/splash.h"

int main(void)
{
    lv_init();
    board_display_init();
    board_input_init();
    ratimos_splash_show();
    while (1) {
        uint32_t idle = lv_timer_handler();
        board_tick(idle);
    }
    return 0;
}
```
Zero `#include <SDL2/SDL.h>` remains — this is the primary verification check (grep for `SDL_` outside `board/native_sdl/`).

---

### `platformio.ini` (build_src_filter edit)

**Analog:** existing `[env:native_sim]` block (`platformio.ini:11-23`).

**Current filter to extend:**
```ini
build_src_filter =
    +<*>
    -<esp32/*>
```
**Target** (per RESEARCH.md, drop the now-vacuous `esp32/*` line, add the new exclusion):
```ini
build_src_filter =
    +<*>
    -<board/waveshare_s3_35/*>
```

---

### `src/storage/content_api.h` (model/interface, CRUD)

**Analog:** `src/ratimos/theme.h` for header conventions (include guard, `#include "lvgl.h"` only if needed, plain function declarations after struct typedefs). No existing "storage" analog in this codebase — this is new infrastructure; follow RESEARCH.md's exact recommended shape (allocation-free, fixed-size caller buffers per D-10/D-13 and Anti-Patterns):
```c
typedef struct {
    char id[16];
    char title[64];
} ratimos_letter_t;

size_t ratimos_storage_list_letters(ratimos_letter_t * out, size_t max_count);
```
Repeat the same `{id, title}`-shaped struct + `list_*` function per domain (photos, tracks, letters, games), plus a `ratimos_settings_t` + `ratimos_storage_get_settings()` for the singleton settings domain. Also declare the staged-init functions used by the splash: `ratimos_storage_mount()`, `ratimos_storage_index_photos()`, `..._tracks()`, `..._letters()`, `..._games()`, `..._settings()`.

---

### `src/storage/photos.c` / `tracks.c` / `letters.c` / `games.c` / `settings.c` (service, CRUD + file-I/O)

**Analog:** No existing analog in this codebase (greenfield layer) — pattern is RESEARCH.md's own synthesis (Pattern 2 + Pitfall 1 + Pitfall 4), which is now the authoritative source. Key rules to copy:
1. **Hardcoded filename manifest**, not `dirent.h` directory scanning (Pitfall 4 — portability + V12 path-traversal discipline):
```c
static const char * const s_letter_files[] = {
    "assets/mock/letters/letter1.txt",
    "assets/mock/letters/letter2.txt",
    "assets/mock/letters/letter3.txt",
};
```
2. **Cache-on-index, not read-on-get** (Pitfall 1): the "index" step (called once from the splash) does the real `fopen`/`fread`/parse into a `static ratimos_letter_t s_letters[4]; static size_t s_letter_count;`. The public `ratimos_storage_list_letters()` getter just `memcpy`s from that static cache — no disk I/O at app-screen-open time.
3. Never write `.bin` extensions for fixtures (Pitfall 2 — colides with existing `.gitignore` `*.bin` rule); use `.txt`/`.json`/`.dat`.
4. Missing/blank title field defaults to `"sem titulo"` (UI-SPEC "partial" row, low-priority defensive default).

**Error handling:** no exceptions in C — return 0 items / a safe default struct on any `fopen` failure; do not crash (UI-SPEC backstop row on splash init failure is optional/low-priority per Copywriting Contract).

---

### `src/ratimos/splash.c` / `.h` (component/screen, event-driven staged init)

**Analog:** `src/ratimos/home_screen.c` for the screen-lifecycle shape (`build_*_screen()` → cache singleton `lv_obj_t*` → `lv_screen_load()` inside a `ratimos_*_show(lv_event_t*)` function matching the existing `(void) e;` no-op-event-arg convention seen in every `apps/*_app.c` file and `home_screen.c:74-81`).

**Core screen-build pattern to imitate** (`src/ratimos/home_screen.c:32-48, 74-81`):
```c
static lv_obj_t * scr = lv_obj_create(NULL);
ratimos_theme_apply_screen(scr);
lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
...
void ratimos_home_screen_show(lv_event_t * e)
{
    (void) e;
    if (!s_home_screen) { s_home_screen = build_home_screen(); }
    lv_screen_load(s_home_screen);
}
```
For the splash, `ratimos_splash_show(void)` builds the screen fresh (no caching needed, it only ever runs once at boot), adds the logo `lv_image_create()` + `lv_obj_fade_in(logo, 2000, 0)` per D-01/UI-SPEC image pipeline, an `lv_bar` widget for progress (per RESEARCH.md "Don't Hand-Roll" table), and an `lv_timer_create(splash_step_cb, ~330, NULL)` implementing RESEARCH.md Pattern 2 verbatim (already a complete code example in RESEARCH.md lines 247-277 — copy that directly). Per D-04, do NOT add any `LV_EVENT_CLICKED`/`LV_OBJ_FLAG_CLICKABLE` on the splash screen — simply omit input wiring (no code needed to "disable" touch).

**Theme reuse:** call `ratimos_theme_apply_screen(scr)` like every other screen (`src/ratimos/theme.c:3-10`) so splash bg matches `RATIMOS_COLOR_BG` (updated per D-17/UI-SPEC to sample-derived hex values in `theme.h`).

---

### `src/ratimos/apps/*.c` (component/screen, CRUD via Storage API)

**Analog:** each app is its own closest analog — same shape, just swap the hardcoded `ratimos_row_create(...)` calls for a loop over `ratimos_storage_list_*()` results, and make the empty-state row conditional on count == 0.

**Existing exact shape to preserve** (`src/ratimos/apps/jogos_app.c:1-16` — representative of all 5):
```c
#include "jogos_app.h"
#include "../app_shell.h"
#include "../row_list.h"

void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("jogos", "toque para abrir");

    ratimos_row_create(shell.content, "S", "sudoku", "abrir", NULL);
    ratimos_row_create(shell.content, "C", "car jam", "abrir", NULL);
    ratimos_row_create(shell.content, "P", "paciencia", "abrir", NULL);

    lv_screen_load(shell.screen);
}
```
**Target shape** (loop + conditional empty state, new `#include "../../storage/content_api.h"`):
```c
void ratimos_jogos_show(lv_event_t * e)
{
    (void) e;
    ratimos_app_shell_t shell = ratimos_app_shell_create("jogos", "toque para abrir");

    ratimos_game_t games[4];
    size_t n = ratimos_storage_list_games(games, 4);
    if (n == 0) {
        ratimos_row_create(shell.content, "!", "nenhum jogo disponivel", "verifique a instalacao do RatimOS", NULL);
    } else {
        for (size_t i = 0; i < n; i++) {
            ratimos_row_create(shell.content, "J", games[i].title, "abrir", NULL);
        }
    }

    lv_screen_load(shell.screen);
}
```
**Cartas empty-state copy already conforms** (`src/ratimos/apps/cartas_app.c:11`) — reuse that exact row call as the `n == 0` branch template: `ratimos_row_create(shell.content, "!", "nenhuma carta ainda", "chegam aqui quando sincronizadas", NULL);`

**Copy-string fixups required per UI-SPEC Copywriting Contract while touching these files:**
- `album_app.c:21` — `"toque pra abrir"` → `"toque para abrir"`
- `cartas_app.c:8` — `"toque pra ler"` → `"toque para abrir"`
- `musica_app.c:8` — `"toque na musica"` → `"toque para abrir"`

**Album grid variant** (`src/ratimos/apps/album_app.c:5-16`) — keep the `photo_tile_create` panel-tile shape, but drive the loop count from `ratimos_storage_list_photos()` instead of the hardcoded `for (int i = 0; i < 4; i++)`, and add `LV_LABEL_LONG_MODE_DOT` to any label rendering a real fixture title (UI-SPEC "overflow/long-text" unresolved item).

**Config app settings variant** (`src/ratimos/apps/config_app.c`) — replace the 4 hardcoded rows with values from `ratimos_storage_get_settings()` (brightness/volume/device-info placeholders per D-09), same `ratimos_row_create` shape.

---

## Shared Patterns

### Screen chrome / navigation (unchanged, reused everywhere)
**Source:** `src/ratimos/app_shell.c:6-30`, `src/ratimos/status_bar.c` (all 3 bar builders), `src/ratimos/home_screen.c`
**Apply to:** splash screen (partially — topbar/sectionbar optional for splash, "one more screen" framing per UI-SPEC 2xl margin note), all 5 app files (no change to shell usage, only to what's populated inside `shell.content`)
```c
ratimos_app_shell_t shell = ratimos_app_shell_create(section_label, hint);
/* populate shell.content via Storage API */
lv_screen_load(shell.screen);
```

### Themed building blocks
**Source:** `src/ratimos/theme.c:12-43` (`ratimos_panel_create`, `ratimos_badge_create`)
**Apply to:** any new tile/row rendering Storage API results (album grid tiles, row-list items) — do not hand-roll new panel/badge styling; reuse these two functions.

### Row list rendering
**Source:** `src/ratimos/row_list.c:4-44`
**Apply to:** cartas/musica/jogos/config apps — `ratimos_row_create(parent, letter, title, subtitle, click_cb)` is the single reusable list-row primitive; add `LV_LABEL_LONG_MODE_DOT` inside `row_list.c`'s `title_lbl`/`sub_lbl` creation (currently missing, UI-SPEC unresolved item) so all callers get truncation for free rather than patching each app file individually.

### Color/theme tokens (palette repaint, D-17)
**Source:** `src/ratimos/theme.h:11-16`
**Apply to:** every screen — update the 6 `RATIMOS_COLOR_*` hex defines to the new UI-SPEC-derived values (`BG #000000`, `PANEL #2a123f`, `PANEL_ACTIVE #4e2277`, `ACCENT #e6010f`, `TEXT #f5f2f8`, `TEXT_MUTED #a997ba`) — single edit point, no per-screen color literals exist elsewhere in the codebase (verified: every screen/app file only references `RATIMOS_COLOR_*` macros, never raw hex).

### Staged/synchronous init, no async plumbing (D-10)
**Source:** RESEARCH.md Pattern 2 (splash timer) — codebase has no prior async/callback pattern to point to; this is the first instance, and it must stay this simple. Do not introduce callbacks/promises/events beyond `lv_timer`.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `src/storage/*.c` (all 5 domain files) | service | CRUD + file-I/O | No prior storage/persistence layer exists in this codebase (Phase 0 was pure hardcoded UI) — pattern sourced entirely from RESEARCH.md's own synthesis (Pattern 2, Pitfalls 1/2/4), not an existing analog. |
| `test/test_storage/test_storage.c` | test | CRUD verification | No `test/` directory exists yet in this repo — first test file; use PlatformIO's documented Unity skeleton (`#include <unity.h>`, `UNITY_BEGIN()/END()`) per RESEARCH.md Validation Architecture, not a codebase analog. |
| `assets/mock/**` fixture files + manifest | data | file-I/O | New asset tree, no prior fixture convention in this repo. |
| `src/board/waveshare_s3_35/board.c` | driver stub | event-driven | No second board implementation has ever existed in this codebase; only close reference is its own sibling `native_sdl/board.c` for signature shape (body content is intentionally divergent — TODO-only). |

## Metadata

**Analog search scope:** `src/main.c`, `src/ratimos/**` (all existing .c/.h files — theme, app_shell, home_screen, row_list, status_bar, all 5 apps), `platformio.ini`
**Files scanned:** 20 existing project source files (excluded `.pio/libdeps` vendored LVGL source, already covered by RESEARCH.md's own verified API citations)
**Pattern extraction date:** 2026-08-27
