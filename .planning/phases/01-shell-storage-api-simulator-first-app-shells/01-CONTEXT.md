# Phase 1: Shell, Storage API & Simulator-First App Shells - Context

**Gathered:** 2026-08-26
**Status:** Ready for planning

<domain>
## Phase Boundary

The RatimOS shell runs end-to-end in the PC simulator (SDL2): a real boot identity/splash before the home screen, navigation between home and all 5 sections (jogos/musica/album/cartas/config), and every app reading its content exclusively through a shared Storage/Content API instead of touching data directly. This phase also establishes the board/HAL split (`board_native_sdl` now, `board_waveshare_s3_35` as a compiling stub) so app code compiles unchanged against both — the real hardware driver implementation is Phase 3's job, not this phase's.

</domain>

<decisions>
## Implementation Decisions

### Boot Identity / Splash
- **D-01:** Splash shows the RatimOS logo/name with a fade-in animation (`lv_anim`).
- **D-02:** Splash lasts ~2s and shows a loading bar tied to real boot progress (not a fake timer).
- **D-03:** The loading bar reflects actual Storage/Content API initialization steps (mount native/mock backend → index mock content) — real feedback, not decoration.
- **D-04:** Touch during the splash does nothing — it always plays to completion, no tap-to-skip.

### HAL / Board Abstraction
- **D-05:** Board interface contract is explicit: `board_display_init()`, `board_input_init()`, `board_tick()` — separate functions, not a single monolithic init. This anticipates Phase 3, where display (ST7796) and touch (FT6336) will have genuinely different init/failure paths.
- **D-06:** Board implementations live in `src/board/native_sdl/` and `src/board/waveshare_s3_35/` — one subfolder per board, sibling structure. — **Reversibility:** costly — every `#include`/build filter referencing the old flat-file layout would need updating if this changes later.
- **D-07:** `board_waveshare_s3_35` is a compiling stub in Phase 1 (implements the interface with TODOs, no real hardware behavior). No `esp32s3` PlatformIO environment is added yet — `platformio.ini` still only builds `native_sim`; the stub just needs to compile as a translation unit to prove the interface holds. Wiring an actual build target for it is out of scope for this phase.
- **D-08:** `main.c` stays a single, generic entry point for both environments — it only calls the board interface functions, with zero SDL2-specific code inline. This is what lets app code "compile unchanged against both" per the ROADMAP's Phase 1 success criteria.

### Storage/Content API
- **D-09:** The API covers 5 content domains: photos (album), tracks (musica), letters (cartas), games list (jogos), and settings (config — brightness/volume/device info placeholders). Settings were pulled into the same API rather than left hardcoded in `config_app.c`, so config isn't a special case.
- **D-10:** The API is synchronous in Phase 1 (e.g. `ratimos_storage_list_photos()` returns directly). Async (callback/event-based) is explicitly deferred to whichever phase first needs it for real (SD I/O latency in Phase 5, network sync in Phase 7) — **do not build async plumbing now**, it would be premature for a synchronous mock backend.
- **D-11:** Lives in `src/storage/` — a sibling of `src/board/`, not nested under `src/ratimos/`. Signals it's a shared infrastructure layer serving all apps equally, not part of the UI layer.

### Conteúdo mockado no simulador
- **D-12:** The native storage backend reads real fixture files from disk (not hardcoded C arrays) — closer to how real SD reads will behave once Phase 5 lands.
- **D-13:** Keep fixture counts small: 3-4 items per content type (photos, tracks, letters, games) — enough to prove list/grid rendering with more than one item, without bloating the repo.
- **D-14:** Fixture content must be generic/placeholder (e.g. "Carta de teste 1", stock-style placeholder images) — **not** real personal content. These fixtures live in the git repository (already published to GitHub), so nothing personal belongs here. Real content only ever arrives via cloud sync starting Phase 7.
- **D-15:** Fixtures live at `assets/mock/` at the project root — read only by the native storage backend when built for `native_sim`. The name makes it unambiguous this is dev-only and must never be pulled into an `esp32s3` build.

### Claude's Discretion
- Exact file layout inside `src/board/native_sdl/` and `src/board/waveshare_s3_35/` (how many files per board, header/source split) — user explicitly delegated this ("você que decide isso, onde for melhor mais organizado com mais desempenho e escalável"). Recommendation: mirror the existing `src/ratimos/*.c/.h` pattern per board (e.g. `board.c`/`board.h` implementing the shared interface).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project & Requirements
- `.planning/PROJECT.md` — project identity, constraints (PlatformIO+Arduino+LVGL stack, simulator-first, no hardware yet), key decisions (Waveshare ESP32-S3-Touch-LCD-3.5 board chosen)
- `.planning/ROADMAP.md` §"Phase 1: Shell, Storage API & Simulator-First App Shells" — goal and the 4 success criteria this phase must satisfy
- `.planning/REQUIREMENTS.md` — SHELL-01 (navigate home ↔ 5 sections), SHELL-03 (boot identity, not stock/blank screen)

### Existing code (already built — Phase 0 baseline)
- `platformio.ini` — `native_sim` environment: SDL2, `-DLV_CONF_INCLUDE_SIMPLE`, `build_src_filter` already excludes `esp32/*` (pattern to extend for the new `board/` split)
- `lv_conf.h` — LVGL 9.x config, color depth 16, SDL driver already wired
- `src/main.c` — current monolithic entry point (calls `lv_sdl_window_create`/`lv_sdl_mouse_create` directly) — this is what gets refactored into the generic main.c + board interface
- `src/ratimos/theme.h` — color palette (`RATIMOS_COLOR_BG`, `RATIMOS_COLOR_ACCENT`, etc.), screen size constants (`RATIMOS_SCREEN_W/H` = 320×480) — reuse for splash styling
- `src/ratimos/home_screen.c`, `src/ratimos/app_shell.c`, `src/ratimos/status_bar.c`, `src/ratimos/row_list.c` — existing navigation shell and shared UI components (topbar/sectionbar/bottombar, row list) to keep working once apps switch to reading through the Storage API
- `src/ratimos/apps/*.c` — all 5 apps currently have inline placeholder content (e.g. `album_app.c` creates 4 empty tiles, `musica_app.c` has a fixed "0 faixas" playlist) — these are exactly what the Storage/Content API replaces

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ratimos_app_shell_create()` (`src/ratimos/app_shell.c`) — standard chrome (topbar/sectionbar/content/bottombar) every app already uses; unaffected by this phase's changes, just gets fed real data instead of hardcoded rows
- `ratimos_row_create()` (`src/ratimos/row_list.c`) — shared list-row component, reusable for rendering Storage API results (letters list, games list, track list)
- `ratimos_panel_create()` / `ratimos_badge_create()` (`src/ratimos/theme.c`) — themed building blocks for tiles (album grid, home tiles)

### Established Patterns
- Every app file follows the same shape: `ratimos_app_shell_create(section_label, hint)` → populate `shell.content` → `lv_screen_load(shell.screen)`. New Storage API calls slot into the "populate content" step without changing this shape.
- Bottom bar "voltar" always returns to `ratimos_home_screen_show` — single global back target, no per-app back stack. Not revisited in this discussion; assumed to hold for Phase 1.
- `build_src_filter` in `platformio.ini` already demonstrates the per-environment file exclusion pattern (`-<esp32/*>`) that the new `board/` split will extend.

### Integration Points
- `src/main.c` is the sole integration point between the board layer and the rest of the app — once split, it becomes the only file that calls `board_display_init`/`board_input_init`/`board_tick`.
- Each `apps/*_app.c` is where Storage/Content API calls get wired in, replacing the current inline placeholder data.

</code_context>

<specifics>
## Specific Ideas

- Splash loading bar must be tied to *real* async-looking work (Storage API mount/index), not a cosmetic timer — user was explicit that this should give "feedback real pro usuário."
- The `.pio/` build directory (92MB) is git-ignored; a `.gitignore` was added at the project root during this session covering `.pio/`, `.vscode/`, `*.o`, `*.bin`.
- The RatimOS project now has its own standalone GitHub repository (separate from the developer's unrelated `OTZ_LibreChat`/home-directory git repo it was previously entangled with) — this happened mid-session via GitHub Desktop, not through this workflow's `git_commit` step.

</specifics>

<deferred>
## Deferred Ideas

- **Boot-time wifi auto-connect + OTA/sync check on the splash screen** — the user asked whether the splash could auto-connect to a previously known wifi network and check for firmware updates / new synced content while loading. This is explicitly out of scope for Phase 1 (simulator-only, no network stack exists yet). Belongs across **Phase 6** (WiFi Provisioning — auto-reconnect to known networks), **Phase 7** (Cloud Content Sync — check for new content), and **Phase 8** (OTA — check for firmware updates). When those phases are planned, revisit the splash screen as a natural place to surface that status.

### Reviewed Todos (not folded)
None — no pending todos matched this phase.

</deferred>

---

*Phase: 1-Shell, Storage API & Simulator-First App Shells*
*Context gathered: 2026-08-26*
