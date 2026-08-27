---
phase: 01-shell-storage-api-simulator-first-app-shells
plan: 1
subsystem: infra
tags: [lvgl, sdl2, platformio, board-hal, storage-api, unity-test, embedded-c]

requires: []
provides:
  - "src/board/board.h contract (board_display_init/input_init/tick) with native_sdl (real) and waveshare_s3_35 (compiling stub) implementations"
  - "src/storage/content_api.h — full 5-domain Storage/Content API declared; letters domain fully real, fixture-backed, cache-on-index"
  - "Real boot splash (ratimos_splash_show): logo fade-in + staged real-progress bar driven by an lv_timer over named storage-init steps"
  - "Compiled-in LVGL RGB565 logo asset (tools/convert_logo.py + logo_image.c/.h)"
  - "cartas_app.c reading exclusively through the Storage API, screen cached (build-once) to avoid the LVGL heap-exhaustion leak"
  - "Wave 0 Unity test scaffold (test/test_storage) covering the letters domain"
affects: [01-02, 01-03, phase-3-hardware-bringup]

actuals:
  tokens: 144100
  tasks: 2
  commits: 7

tech-stack:
  added: []
  patterns:
    - "Board/HAL split: main.c only calls board_display_init()/board_input_init()/board_tick() — zero SDL2 code outside src/board/native_sdl/"
    - "Storage API: allocation-free, fixed-size caller buffers, cache-on-index (index_* populates a static cache once; list_* only memcpy's from it)"
    - "Screen-cache-once pattern (mirrors ratimos_home_screen_show()): build a screen once, lv_screen_load() the cached instance on every subsequent visit — required to avoid leaking a new lv_obj_t screen per navigation"
    - "Splash step table (splash_step_t[] + SPLASH_TOTAL_MS/SPLASH_STEP_COUNT) — extensible: future domains append entries without touching timer logic"

key-files:
  created:
    - src/board/board.h
    - src/board/native_sdl/board.c
    - src/board/waveshare_s3_35/board.c
    - src/storage/content_api.h
    - src/storage/storage.c
    - src/storage/letters.c
    - src/ratimos/splash.c
    - src/ratimos/splash.h
    - src/ratimos/logo_image.c
    - src/ratimos/logo_image.h
    - tools/convert_logo.py
    - test/test_storage/test_storage.c
    - assets/mock/letters/carta1.txt
    - assets/mock/letters/carta2.txt
    - assets/mock/letters/carta3.txt
  modified:
    - platformio.ini
    - src/main.c
    - src/ratimos/apps/cartas_app.c
    - lv_conf.h

key-decisions:
  - "cartas_app.c caches its built screen (build-once, like home_screen.c) instead of rebuilding on every visit — required to stop an LVGL builtin-heap exhaustion crash reproduced after ~3-4 repeated visits."
  - "lv_conf.h: LV_MEM_SIZE raised from LVGL's 64KB default to 512KB for native_sim only — PC has abundant RAM; Phase 3's esp32s3 environment must define its own hardware-measured value."
  - "jogos/musica/album/config still share the pre-existing screen-rebuild-every-visit pattern that caused the cartas crash — logged as a deferred item (deferred-items.md) for plan 01-03 rather than fixed now (out of this plan's file scope)."

patterns-established:
  - "Pattern 1: main.c stays a single generic entry point; only board.h's 3 functions + lv_init/lv_timer_handler may appear in it."
  - "Pattern 2: splash-driven staged storage init — an lv_timer advances through named, real init steps, updating a progress bar from actual completed work, never a cosmetic timer."

requirements-completed: [SHELL-01, SHELL-03]

coverage:
  - id: D1
    description: "Board/HAL split: zero SDL2 calls outside src/board/native_sdl/, native_sim links only the native_sdl implementation (waveshare stub excluded via build_src_filter)"
    requirement: "SHELL-01"
    verification:
      - kind: unit
        ref: "grep -rn 'SDL_' src | grep -v src/board/native_sdl/ (zero matches)"
        status: pass
      - kind: integration
        ref: "pio run -e native_sim"
        status: pass
    human_judgment: false
  - id: D2
    description: "waveshare_s3_35 board stub compiles standalone with plain gcc, no ESP-IDF/Arduino includes (D-07)"
    verification:
      - kind: unit
        ref: "gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c"
        status: pass
    human_judgment: false
  - id: D3
    description: "Storage API letters domain: 3 real fixture-backed letters returned, correct titles, max_count respected"
    verification:
      - kind: unit
        ref: "test/test_storage/test_storage.c#test_letters_list_returns_fixture_titles"
        status: pass
      - kind: unit
        ref: "test/test_storage/test_storage.c#test_letters_list_respects_max_count"
        status: pass
    human_judgment: false
  - id: D4
    description: "Boot splash: real logo fade-in + genuine staged progress bar (driven by real storage-init steps) before home screen, no tap-to-skip; cartas renders the 3 real letters end-to-end through the splash's own indexing"
    requirement: "SHELL-03"
    verification:
      - kind: manual_procedural
        ref: "Coordinator visual confirmation during checkpoint review — 3 real letter titles rendered, no crash on repeated visits"
        status: pass
    human_judgment: true
    rationale: "Visual timing/fade/no-skip behavior of an LVGL splash cannot be meaningfully asserted by a headless unit test in this codebase (no LVGL headless-display harness exists) — confirmed via human checkpoint verification and an internal (non-committed) widget-tree-walking repro during this plan's execution, per RESEARCH.md's Validation Architecture."

duration: 55min
completed: 2026-08-27
status: complete
---

# Phase 1 Plan 1: Board/HAL Split, Storage API Core & Boot Splash Summary

**Board/HAL split (native_sdl + compiling waveshare_s3_35 stub), a synchronous allocation-free Storage/Content API with a fully real fixture-backed letters domain, and a real boot splash (logo fade-in + staged storage-init progress bar) that hands off to a "cartas" screen reading exclusively through the Storage API — the phase's foundational tracer, proven end-to-end.**

## Performance

- **Duration:** ~55 min (first commit 12:12, last commit 12:56, plus pre-commit investigation/diagnosis time)
- **Started:** 2026-08-27T12:00Z (approx.)
- **Completed:** 2026-08-27T12:56:14-03:00
- **Tasks:** 2 (both completed; Task 1 required 2 checkpoint-driven fix rounds before approval)
- **Files modified:** 19 (15 created, 4 modified across the plan; see key-files)

## Accomplishments

- Extracted every SDL2 call out of `src/main.c` into `src/board/native_sdl/board.c` behind a 3-function `board.h` contract (D-05/D-08); added a compiling-only `waveshare_s3_35` stub (D-07) excluded from `native_sim`'s link via `build_src_filter`.
- Declared the full 5-domain Storage/Content API (`content_api.h`) with the `letters` domain fully real: hardcoded fixture manifest (never `dirent.h`, per RESEARCH.md's path-traversal discipline), cache-on-index, safe `"sem titulo"` fallback.
- Built a real boot splash (`ratimos_splash_show`): RatimOS logo (compiled-in RGB565 asset via `tools/convert_logo.py`) fades in, a progress bar advances through 2 real named storage-init steps via an extensible step table, then auto-hands-off to home — no click ever wired on the splash screen (D-04).
- `cartas_app.c` renders exclusively through `ratimos_storage_list_letters()`, showing the 3 real fixture letters end-to-end (splash indexes them, cartas lists them).
- Added a Wave 0 Unity test suite (`test/test_storage`) covering the letters domain's count and max-count-capping behavior.

## Task Commits

Each task was committed atomically (Task 1 required 2 additional fix commits after checkpoint review, per the deviations below):

1. **Task 1: End-to-end boot — board split, storage core & real "cartas" letters** — `5ad7c00` (feat)
   - Checkpoint fix 1: `fb24437` (fix) — cartas screen-leak crash
   - Checkpoint fix 2: `ab6503d` (fix) — missing storage mount/index wiring
   - `d4311f9` (docs) — logged the remaining cross-app leak as a deferred item
2. **Task 2: Boot splash, compiled-in logo asset & Wave 0 storage test scaffold** — 3 commits:
   - `4ec882a` (feat) — compiled-in logo asset
   - `a62d319` (feat) — real splash wiring
   - `8cee27c` (test) — Wave 0 storage test coverage

**Plan metadata:** (this commit, docs: complete plan)

## Files Created/Modified

- `src/board/board.h` — shared 3-function board/HAL contract
- `src/board/native_sdl/board.c` — SDL2 implementation (all SDL2 calls extracted from main.c)
- `src/board/waveshare_s3_35/board.c` — compiling TODO-only stub, zero ESP-IDF/Arduino includes
- `src/storage/content_api.h` — full 5-domain Storage API contract (structs + function declarations)
- `src/storage/storage.c` — `ratimos_storage_mount()`
- `src/storage/letters.c` — real, fixture-backed letters domain (cache-on-index)
- `assets/mock/letters/carta{1,2,3}.txt` — generic placeholder letter fixtures
- `src/ratimos/apps/cartas_app.c` — reads via Storage API; screen cached (build-once) after the leak fix
- `src/ratimos/splash.c` / `.h` — boot splash: fade-in + staged real-progress bar
- `src/ratimos/logo_image.c` / `.h` — compiled-in RGB565 logo asset (generated)
- `tools/convert_logo.py` — one-off PNG→RGB565 conversion script
- `test/test_storage/test_storage.c` — Wave 0 Unity tests for the letters domain
- `platformio.ini` — `build_src_filter` extended to exclude the waveshare stub; `test_build_src = yes` added
- `src/main.c` — generic entry point calling `board_display_init/input_init/tick` + `ratimos_splash_show()`; `main()` guarded with `#ifndef UNIT_TEST`
- `lv_conf.h` — `LV_MEM_SIZE` raised to 512KB (native_sim only)

## Decisions Made

- **Screen-cache-once for cartas_app.c:** mirrors the existing `ratimos_home_screen_show()` convention (build once, `lv_screen_load()` the cached instance) rather than rebuilding on every visit — both correct (letters only ever indexed once, D-10) and required (eliminates the leak that crashed the app).
- **LV_MEM_SIZE bump (512KB, native_sim only):** LVGL's builtin-allocator default (64KB) was never tuned in this repo's `lv_conf.h` and is undersized for a retained multi-screen widget UI; bumped for headroom as defense-in-depth since 4 other app screens still share the same leak pattern.
- **Temporary main.c bridge, then removed:** during Task 1's checkpoint fix, `ratimos_storage_mount()/index_letters()` were called directly from `main()` as a minimal bridge so Task 1's own claim (cartas renders real Storage-API-backed content) held true of the running binary before the splash existed. Task 2 cleanly replaced this bridge with `ratimos_splash_show()`, whose own step table now owns that responsibility — no leftover redundancy.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Cartas screen leak exhausting LVGL's builtin heap after repeated visits**
- **Found during:** Task 1 checkpoint verification (coordinator's live click-through crashed the simulator)
- **Issue:** `ratimos_cartas_show()` built a brand-new `lv_obj_t` screen on every visit with no caching/deletion of the previous one (a pattern shared by every other `apps/*.c` file, pre-existing since Phase 0). Combined with LVGL's un-tuned 64KB builtin heap default, this exhausted memory after ~3-4 repeated visits, producing `lv_realloc: couldn't reallocate memory` / `lv_array_resize` assert crashes.
- **Fix:** Cached the built cartas screen (build-once, like `home_screen.c`); raised `LV_MEM_SIZE` to 512KB in `lv_conf.h` as defense-in-depth for the other 4 apps' still-unfixed instance of the same pattern (logged as a deferred item, not fixed — out of this plan's file scope).
- **Files modified:** `src/ratimos/apps/cartas_app.c`, `lv_conf.h`
- **Verification:** Reproduced the exact crash deterministically (visit #4) in a headless harness driving the real `board.c`/SDL/LVGL code path before the fix; confirmed completely flat memory usage across 20 repeated visits after the fix; re-verified live by the coordinator.
- **Committed in:** `fb24437`

**2. [Rule 1 - Bug] Storage mount/index never actually invoked, so cartas showed the empty state instead of real letters**
- **Found during:** Task 1 checkpoint re-verification (after the leak fix, coordinator found cartas showing the empty state, not the 3 real letters required by Task 1's acceptance criteria)
- **Issue:** `ratimos_storage_mount()`/`ratimos_storage_index_letters()` were fully implemented but never called anywhere in the actual program path — `main.c` went straight to `ratimos_home_screen_show(NULL)` by Task 1's own explicit design (splash wiring, the only planned call site, is Task 2's job). Verified directly (not guessed): storage list/index logic and fixture path resolution were both independently confirmed correct; the sole gap was the missing call site.
- **Fix:** Added a temporary, narrowly-scoped `ratimos_storage_mount(); ratimos_storage_index_letters();` bridge in `main()` for Task 1's checkpoint to hold true of the running binary. Cleanly removed and replaced by Task 2's real splash wiring (which drives the same two calls via its own step table).
- **Files modified:** `src/main.c` (temporary in Task 1's fix commit, then replaced in Task 2's splash commit)
- **Verification:** Live re-verification (real board.c/SDL code path, real `LV_EVENT_CLICKED` dispatch, widget-tree walk) confirmed the 3 real titles render; re-confirmed visually by the coordinator.
- **Committed in:** `ab6503d` (temporary bridge); superseded by `a62d319` (real splash wiring)

**3. [Rule 3 - Blocking] PlatformIO native test runner duplicate-`main()` link error**
- **Found during:** Task 2, first `pio test -e native_sim -f test_storage` run
- **Issue:** `test_build_src = yes` (added per the plan's own contingency) pulls all of `src/` — including `main.c`'s `main()` — into the test binary alongside `test_storage.c`'s own `main()`, causing a duplicate-symbol link error.
- **Fix:** Guarded `src/main.c`'s `main()` with `#ifndef UNIT_TEST` — the macro PlatformIO's native test runner auto-defines for test builds. This is a documented, standard PlatformIO native-testing convention, not project-specific.
- **Files modified:** `src/main.c`
- **Verification:** `pio test -e native_sim -f test_storage` passes (2/2); `pio run -e native_sim` (the real app build) unaffected.
- **Committed in:** `8cee27c`

**4. [Rule 2 - Missing critical, environment] `libsdl2-dev` not installed system-wide in this execution sandbox**
- **Found during:** Task 1, first `pio run -e native_sim`
- **Issue:** This sandbox has only the SDL2 runtime library (`libsdl2-2.0-0`), not the `-dev` headers, and no passwordless sudo to install it.
- **Fix:** No project file was changed for this — verified every build/test command locally by downloading the `.deb` (`apt-get download`, no root required) into the scratchpad directory and pointing `CPATH`/`LIBRARY_PATH` at its extracted contents for each verification command only. Nothing was installed system-wide; `platformio.ini` was not modified to depend on this workaround.
- **Files modified:** none (verification-only workaround, entirely outside the repo)
- **Verification:** All builds/tests in this SUMMARY were run and passed via this workaround. On a machine with `libsdl2-dev` properly installed (confirmed by the coordinator during checkpoint review), `pio run -e native_sim` / `pio test` work standard, no special env vars needed.

---

**Total deviations:** 4 auto-fixed (3 bugs/blocking issues, 1 environment workaround). All directly caused by or required to prove correct this plan's own changes; no scope creep beyond what checkpoint verification surfaced.
**Impact on plan:** All fixes necessary for the tracer to actually hold true (build, tests, and live behavior all green); the remaining cross-app screen-leak pattern (jogos/musica/album/config) was deliberately left unfixed and logged in `deferred-items.md` for plan 01-03, per the scope-boundary rule.

## Issues Encountered

- Reproducing the reported live crash required building 7 progressively more faithful headless harnesses (direct function call → real LVGL event dispatch → real `board.c`/SDL code path → repeated-visit stress test) since the bug only manifested after several repeated screen visits, not a single navigation. The final harness deterministically reproduced the exact reported error signature at visit #4, giving high confidence in the diagnosis before writing the fix.
- One shell-quoting mistake (embedded double-quotes inside a double-quoted `git commit -m` string) truncated a commit message mid-sentence; corrected via `git commit --amend` on that same still-unpushed, no-dependents commit before it was reported to the coordinator.

## User Setup Required

None - no external service configuration required. (Local dev-environment note: this execution sandbox lacked `libsdl2-dev`; see Deviation 4 above. Not applicable to the user's own machine, which the coordinator confirmed already has it installed.)

## Next Phase Readiness

- The tracer holds end-to-end: board split, Storage API core, and splash-driven staged init are all proven on one real path (letters → cartas) exactly as this plan intended, de-risking plan 01-03's expansion to the remaining 4 domains/apps.
- **Carry-forward for 01-03 (or a dedicated hardening pass):** apply the same screen-cache-once fix already applied to `cartas_app.c` to `jogos_app.c`, `musica_app.c`, `album_app.c`, and `config_app.c` — see `.planning/phases/01-shell-storage-api-simulator-first-app-shells/deferred-items.md`. Low severity on the PC simulator (abundant RAM even with the leak) but must be resolved or re-measured before Phase 3 hardware bring-up, where RAM is genuinely constrained.
- `LV_MEM_SIZE` is currently sized generously for `native_sim` only; Phase 3 must define its own hardware-measured value for the `esp32s3` environment (not yet created).

## Self-Check: PASSED

- `src/board/board.h` — FOUND
- `src/board/native_sdl/board.c` — FOUND
- `src/board/waveshare_s3_35/board.c` — FOUND
- `src/storage/content_api.h` — FOUND
- `src/storage/storage.c` — FOUND
- `src/storage/letters.c` — FOUND
- `src/ratimos/splash.c` — FOUND
- `src/ratimos/splash.h` — FOUND
- `src/ratimos/logo_image.c` — FOUND
- `src/ratimos/logo_image.h` — FOUND
- `tools/convert_logo.py` — FOUND
- `test/test_storage/test_storage.c` — FOUND
- `assets/mock/letters/carta1.txt`, `carta2.txt`, `carta3.txt` — FOUND
- Commit `5ad7c00` — FOUND in `git log --oneline`
- Commit `fb24437` — FOUND in `git log --oneline`
- Commit `ab6503d` — FOUND in `git log --oneline`
- Commit `d4311f9` — FOUND in `git log --oneline`
- Commit `4ec882a` — FOUND in `git log --oneline`
- Commit `a62d319` — FOUND in `git log --oneline`
- Commit `8cee27c` — FOUND in `git log --oneline`

---
*Phase: 01-shell-storage-api-simulator-first-app-shells*
*Completed: 2026-08-27*
