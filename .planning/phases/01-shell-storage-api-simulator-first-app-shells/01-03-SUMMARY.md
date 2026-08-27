---
phase: 01-shell-storage-api-simulator-first-app-shells
plan: 3
subsystem: infra
tags: [lvgl, storage-api, unity-test, embedded-c, phase-gate]

requires:
  - phase: 01-shell-storage-api-simulator-first-app-shells (plan 01-01)
    provides: board/HAL split, Storage API core (letters domain real), boot splash step-table mechanism, screen-cache-once pattern (cartas_app.c)
  - phase: 01-shell-storage-api-simulator-first-app-shells (plan 01-02)
    provides: D-17 logo-sampled palette (theme.h), row_list.c ellipsis truncation (LV_LABEL_LONG_MODE_DOTS)
provides:
  - "src/storage/photos.c, tracks.c, games.c, settings.c — the 4 remaining Storage/Content API domains, completing all 5 (D-09)"
  - "6 new fixture files under assets/mock/photos/ and assets/mock/tracks/"
  - "src/ratimos/splash.c staging all 6 real init steps (mount + 5 domain indexes)"
  - "jogos/musica/album/config apps reading exclusively through the Storage API, zero hardcoded content rows remaining in any of the 5 app files"
  - "Screen-cache-once leak fix (from 01-01's cartas_app.c) applied to jogos_app.c/musica_app.c/album_app.c/config_app.c — closes the deferred cross-app heap-exhaustion pattern"
  - "test/test_storage/test_storage.c expanded from 2 to 7 Unity tests covering all 5 storage domains"
affects: [phase-3-hardware-bringup]

actuals:
  tokens: 6000
  tasks: 3
  commits: 2

tech-stack:
  added: []
  patterns:
    - "Storage API domain shape repeated 3 more times: hardcoded filename manifest + cache-on-index + safe-default-title (mirrors letters.c from 01-01)"
    - "Compiled-in (non-file-backed) domain variant: games.c has no fopen/manifest, just a static const title array copied into caller buffers on every list call — the one domain that legitimately skips the file-I/O pattern (games ship with firmware, not synced content)"
    - "Screen-cache-once (mirrors home_screen.c/cartas_app.c): every app screen now builds its lv_obj_t once and reuses it, eliminating the shared cross-app heap-exhaustion leak logged in deferred-items.md"

key-files:
  created:
    - src/storage/photos.c
    - src/storage/tracks.c
    - src/storage/games.c
    - src/storage/settings.c
    - assets/mock/photos/foto1.txt
    - assets/mock/photos/foto2.txt
    - assets/mock/photos/foto3.txt
    - assets/mock/tracks/faixa1.txt
    - assets/mock/tracks/faixa2.txt
    - assets/mock/tracks/faixa3.txt
  modified:
    - src/ratimos/splash.c
    - test/test_storage/test_storage.c
    - src/ratimos/apps/jogos_app.c
    - src/ratimos/apps/musica_app.c
    - src/ratimos/apps/album_app.c
    - src/ratimos/apps/config_app.c

key-decisions:
  - "photos.c is metadata-only (title/id read from a .txt fixture's first line) per RESEARCH.md Pitfall 3 — no image-decode dependency added; the existing placeholder-tile pattern in album_app.c is reused, just now populated with a real title instead of a hardcoded 'sem foto' string."
  - "games.c deliberately does NOT follow the file-backed manifest pattern of the other domains — games ship compiled into firmware (D-09's own framing), so index_games() is a documented no-op and list_games() copies from a static const title array."
  - "Extracted the letters.c-style read-title-or-default logic into tracks.c as a non-static (externally linked) function specifically so its safe-default-title defensive branch could be unit-tested directly (Test 5), without corrupting any of the 3 real track fixtures the phase-gate UAT depends on seeing rendered intact."
  - "config_app.c's bottom-right hint string was left as the original 'fase 0 local' rather than changed to the canonical 'toque para abrir' — the UI-SPEC's hint-string fixup only applies to album_app.c/musica_app.c, and config's rows are non-clickable settings displays (click_cb NULL), so 'toque para abrir' would be semantically wrong there."
  - "Applied the build-once screen-cache pattern (from 01-01's cartas_app.c fix) to all 4 remaining app files while wiring them to the Storage API, per the deferred-items.md carry-forward note and this plan's explicit success criteria — closing the cross-app LVGL heap-exhaustion leak instead of deferring it further."

patterns-established:
  - "All 5 Storage API domains (photos/tracks/letters/games/settings) now follow one of two shapes: file-backed cache-on-index (photos/tracks/letters) or compiled-in static list (games); settings is a singleton record, not a list."
  - "All 5 app screens (jogos/musica/album/cartas/config) now follow the identical build-once-cache-then-lv_screen_load shape — no app in this codebase rebuilds its screen on repeat visits anymore."

requirements-completed: [SHELL-01, SHELL-03]

coverage:
  - id: D1
    description: "All 5 storage domains implemented and covered by passing Unity tests; splash stages all 6 real init steps"
    requirement: SHELL-01
    verification:
      - kind: unit
        ref: "pio test -e native_sim -f test_storage (7/7 PASS: 2 from 01-01 + 5 new — photos, tracks, games, settings, tracks safe-default-title)"
        status: pass
      - kind: unit
        ref: "grep -c \"ratimos_storage_index_photos\\|ratimos_storage_index_tracks\\|ratimos_storage_index_games\\|ratimos_storage_index_settings\" src/ratimos/splash.c -> 4"
        status: pass
      - kind: other
        ref: "git ls-files assets/mock/photos/ assets/mock/tracks/ -> all 6 new .txt fixtures present"
        status: pass
      - kind: integration
        ref: "pio run -e native_sim"
        status: pass
    human_judgment: false
  - id: D2
    description: "All 5 apps read exclusively through the Storage/Content API; zero hardcoded content rows remain in any app file; canonical hint string fixups applied where required"
    requirement: SHELL-01
    verification:
      - kind: other
        ref: "grep -rln \"ratimos_storage_list_\\|ratimos_storage_get_settings\" src/ratimos/apps/ -> all 5 app files listed (jogos, musica, album, cartas, config)"
        status: pass
      - kind: other
        ref: "grep -c \"toque para abrir\" src/ratimos/apps/album_app.c src/ratimos/apps/musica_app.c -> both 1; grep for old strings ('toque pra abrir'/'toque na musica') -> zero matches"
        status: pass
      - kind: integration
        ref: "pio run -e native_sim"
        status: pass
    human_judgment: false
  - id: D3
    description: "Phase-gate manual UAT: complete boot-to-navigation flow (splash -> home -> all 5 sections -> back) confirmed against real Storage-API-backed content, in the new D-17 palette"
    requirement: SHELL-03
    verification:
      - kind: manual_procedural
        ref: "Coordinator relayed explicit user approval: splash (real logo fade-in + progress bar) confirmed, all 7 UAT checks passed (splash timing/no-skip, D-17 palette, jogos 3 rows, musica singular/plural playlist + 3 tracks, album 3 titled tiles with ellipsis truncation, cartas 3 letters, config real settings values, voltar navigation from every screen)"
        status: pass
    human_judgment: true
    rationale: "Visual timing/fade/palette/truncation behavior of an LVGL simulator UI cannot be meaningfully asserted by a headless unit test in this codebase (no LVGL headless-display harness exists) — confirmed via the plan's mandatory checkpoint:human-verify phase gate, per RESEARCH.md's Validation Architecture. The user explicitly confirmed all 7 checks, including the splash's real logo fade-in and progress bar."

duration: 25min
completed: 2026-08-27
status: complete
---

# Phase 1 Plan 3: Storage API Expansion, Remaining App Wiring & Phase-Gate UAT Summary

**Completed the Storage/Content API to all 5 domains (photos/tracks/games/settings joining letters), wired the remaining 4 apps (jogos/musica/album/config) to read exclusively through it, closed the cross-app LVGL screen-retention leak deferred from 01-01, and passed the phase's final human-verified UAT — completing Phase 1.**

## Performance

- **Duration:** ~25 min (first commit 16:05, last code commit 16:08, plus context-loading/verification/checkpoint-wait time)
- **Started:** 2026-08-27T~15:50-03:00 (approx.)
- **Completed:** 2026-08-27T16:08:11-03:00 (code); checkpoint approved shortly after
- **Tasks:** 3 (2 auto tasks + 1 checkpoint:human-verify, all completed/approved)
- **Files modified:** 16 (10 created, 6 modified)

## Accomplishments

- Implemented `src/storage/photos.c` and `tracks.c`, mirroring `letters.c`'s exact hardcoded-manifest + cache-on-index + safe-default-title pattern; both are metadata-only (title/id from a fixture's first line), no image/audio decode dependency added.
- Implemented `src/storage/games.c` as the one domain that deliberately does NOT read from disk — games ship compiled into firmware, so `ratimos_storage_index_games()` is a documented no-op and `ratimos_storage_list_games()` copies from a static const title array (`sudoku`, `car jam`, `paciencia`).
- Implemented `src/storage/settings.c` with real, non-zero placeholder values (brightness 80%, volume 50%, firmware `0.1.0-sim`, storage-used label) per D-09.
- Added 6 new generic/placeholder fixture files under `assets/mock/photos/` and `assets/mock/tracks/` (D-14 — no real personal content).
- Extended `src/ratimos/splash.c`'s step table from 2 to 6 entries — the boot progress bar now genuinely reflects all 5 real domain-index steps plus mount.
- Extended `test/test_storage/test_storage.c` from 2 to 7 Unity tests, all passing.
- Wired `jogos_app.c`, `musica_app.c`, `album_app.c`, and `config_app.c` to read exclusively through the Storage API — zero hardcoded content rows remain in any of the 5 app files project-wide.
- Fixed the canonical "toque para abrir" hint string in `album_app.c` and `musica_app.c` (previously "toque pra abrir" / "toque na musica").
- Fixed the singular/plural playlist subtitle in `musica_app.c` ("1 faixa" vs "N faixas"), never a hardcoded "0 faixas".
- Added `LV_LABEL_LONG_MODE_DOTS` truncation to `album_app.c`'s photo tile title label.
- Applied the build-once screen-cache pattern (established in 01-01's `cartas_app.c` fix) to all 4 remaining app files, closing the deferred cross-app LVGL heap-exhaustion leak documented in `deferred-items.md`.
- Passed the full phase-gate manual UAT: user explicitly confirmed the splash's real logo fade-in + progress bar and all 7 checkpoint verification steps.

## Task Commits

Each auto task was committed atomically:

1. **Task 1: Expand Storage API to photos/tracks/games/settings + fixtures + splash step wiring** — `0fc76d0` (feat)
2. **Task 2: Wire musica/album/jogos/config apps to the Storage API** — `fbef789` (feat)
3. **Task 3: Final phase-gate manual UAT** — checkpoint, no code commit (human-verify only); approved by the user via the coordinator.

**Plan metadata:** this commit (docs: complete plan)

_Note: no TDD-per-task RED/GREEN/REFACTOR gate commits — Task 1 carried `tdd="true"` at the task level but was executed as a single cohesive `feat` commit (implementation + its own Unity tests together), matching the pattern already established in 01-01's Task 1._

## Files Created/Modified

- `src/storage/photos.c` — photos domain: file-backed, cache-on-index, metadata-only
- `src/storage/tracks.c` — tracks domain: file-backed, cache-on-index; also exposes `ratimos_storage_tracks_read_title_or_default()` (external linkage, not part of `content_api.h`) so the safe-default-title behavior is directly unit-testable
- `src/storage/games.c` — games domain: compiled-in static title list, no file I/O
- `src/storage/settings.c` — settings domain: singleton real-placeholder record
- `assets/mock/photos/foto{1,2,3}.txt`, `assets/mock/tracks/faixa{1,2,3}.txt` — 6 new generic fixtures
- `src/ratimos/splash.c` — `s_steps[]` extended to 6 entries (mount + 5 domain indexes)
- `test/test_storage/test_storage.c` — 5 new tests appended (7 total)
- `src/ratimos/apps/jogos_app.c` — real 3-game list, build-once screen cache
- `src/ratimos/apps/musica_app.c` — real track list + singular/plural subtitle, canonical hint, build-once screen cache
- `src/ratimos/apps/album_app.c` — real photo tiles with truncating titles, canonical hint, build-once screen cache
- `src/ratimos/apps/config_app.c` — real settings rows (brightness/volume/firmware/storage), build-once screen cache

## Decisions Made

- **photos.c stays metadata-only:** confirms the Open Question already resolved in RESEARCH.md — no JPEG/image decode is in scope this phase; the existing placeholder-tile visual pattern is fed real titles instead.
- **games.c intentionally skips the file-backed manifest pattern:** the only domain of the 5 where "index" is a no-op, since games ship with firmware rather than being synced/fixture content — a deliberate, documented deviation from the otherwise-uniform domain shape, not an oversight.
- **Safe-default-title test (Test 5) implemented via a non-static tracks.c helper, not by corrupting a real fixture:** all 3 real photo/track fixtures needed intact, correct titles for the phase-gate UAT ("Foto de teste 1/2/3" / playlist count), so the defensive "sem titulo" fallback branch is tested directly against a nonexistent path via an externally-linked (but not `content_api.h`-exposed) helper function, rather than by blanking out a committed fixture's first line.
- **config_app.c's hint string left unchanged ("fase 0 local"):** the UI-SPEC's canonical "toque para abrir" fixup was scoped to album/musica only; config's rows are non-clickable settings displays, so a tap-to-open hint would misrepresent the screen's actual interaction model.
- **Screen-cache-once fix folded into this plan's Task 2 rather than deferred further:** per `deferred-items.md`'s own recommendation ("plan 01-03 wires musica/album/jogos/config to the Storage API's remaining domains — a natural point to apply this fix at the same time") and this plan's explicit success criteria, the same build-once pattern from `cartas_app.c` (01-01) was applied to all 4 files while they were already being touched for the Storage API wiring — closing the leak instead of letting it carry into Phase 3 hardware bring-up where RAM is genuinely constrained.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] Applied the deferred screen-cache-once leak fix to all 4 remaining app files**
- **Found during:** Task 2 (explicitly required by the plan's `<critical_note>` and this phase's `deferred-items.md`, not a surprise discovery, but tracked here as a deviation from the plan's literal `<action>` text, which described only the Storage API wiring, not the caching fix)
- **Issue:** `jogos_app.c`, `musica_app.c`, `album_app.c`, and `config_app.c` all rebuilt a brand-new `lv_obj_t` screen on every visit with no caching — the exact pattern that crashed `cartas_app.c` in 01-01 before its fix. Left unfixed, repeated navigation to any of these 4 screens during the phase-gate UAT (which explicitly exercises "voltar" round-trips through every section) risked reproducing the same heap-exhaustion crash.
- **Fix:** Applied the identical build-once-cache-then-`lv_screen_load()` pattern already proven in `cartas_app.c`/`home_screen.c` to all 4 files.
- **Files modified:** `src/ratimos/apps/jogos_app.c`, `musica_app.c`, `album_app.c`, `config_app.c`
- **Verification:** `pio run -e native_sim` builds cleanly; binary launched and ran its main loop without crashing (5s smoke test, killed by timeout not by crash); user's phase-gate UAT explicitly exercised repeated navigation through all 5 sections without incident.
- **Committed in:** `fbef789` (Task 2 commit)

**2. [Rule 3 - Blocking] `pio test` clobbering the `native_sim` app build artifact**
- **Found during:** Pre-checkpoint verification, when a post-`pio test` launch of `.pio/build/native_sim/program` unexpectedly printed Unity test output instead of running the LVGL app
- **Issue:** Running `pio test -e native_sim -f test_storage` after `pio run -e native_sim` overwrites the shared `.pio/build/native_sim/program` artifact with the test binary (both build into the same environment's output path), so a stale test binary could otherwise have been handed to the user for the phase-gate UAT.
- **Fix:** Re-ran `pio run -e native_sim` (full rebuild) immediately before the final smoke-test launch, confirming the real app binary was in place before the checkpoint was handed off.
- **Files modified:** none (build-sequencing fix only, no source change)
- **Verification:** Re-launched the freshly rebuilt binary; it entered its `lv_timer_handler()`/`board_tick()` main loop and ran for the full smoke-test duration with no crash or stray test output.
- **Committed in:** n/a (no source change)

---

**Total deviations:** 2 auto-fixed (1 missing-critical-functionality fix required by the plan's own explicit carry-forward note, 1 blocking build-sequencing issue). No scope creep beyond what the plan and its cross-referenced `deferred-items.md` already called for.
**Impact on plan:** Both fixes were necessary for the plan's own stated success criteria (leak-free navigation, a correct binary handed to the phase-gate UAT) to actually hold true; no architectural changes, no unresolved issues carried forward.

## Issues Encountered

None beyond the two auto-fixed items above.

## User Setup Required

None — no external service configuration required. The user's own machine already has `libsdl2-dev` installed (confirmed during 01-01's checkpoint review).

## Next Phase Readiness

- Phase 1 is now complete: all 4 ROADMAP success criteria hold end-to-end (splash before home, 5-section navigation, storage-API-only content across all 5 apps, board/HAL structural parity) — confirmed by the user's explicit phase-gate approval.
- The cross-app LVGL screen-retention leak flagged in `deferred-items.md` is now fully closed — all 5 app screens (jogos/musica/album/cartas/config) use the identical build-once-cache pattern; no further carry-forward needed on this item.
- `LV_MEM_SIZE` remains sized generously (512KB) for `native_sim` only, per 01-01's note — Phase 3 must still define its own hardware-measured value for the (not-yet-created) `esp32s3` environment; this is unaffected by this plan's leak fix (the fix removes the leak's growth rate entirely, it doesn't change the heap budget question for real hardware).
- All 5 Storage/Content API domains are now real and fully test-covered — Phase 5 (real SD-backed storage) can extend/replace `src/storage/*.c`'s current mock-file-backed implementations behind the same `content_api.h` contract without touching any app code.

## Self-Check: PASSED

- `src/storage/photos.c` — FOUND
- `src/storage/tracks.c` — FOUND
- `src/storage/games.c` — FOUND
- `src/storage/settings.c` — FOUND
- `assets/mock/photos/foto1.txt`, `foto2.txt`, `foto3.txt` — FOUND
- `assets/mock/tracks/faixa1.txt`, `faixa2.txt`, `faixa3.txt` — FOUND
- `src/ratimos/splash.c` — FOUND (6-entry step table confirmed)
- `test/test_storage/test_storage.c` — FOUND (7 tests confirmed passing)
- `src/ratimos/apps/jogos_app.c`, `musica_app.c`, `album_app.c`, `config_app.c` — FOUND (Storage API wiring + screen-cache confirmed)
- Commit `0fc76d0` — FOUND in `git log --oneline`
- Commit `fbef789` — FOUND in `git log --oneline`

---
*Phase: 01-shell-storage-api-simulator-first-app-shells*
*Completed: 2026-08-27*
