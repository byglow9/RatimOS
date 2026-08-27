---
phase: 01-shell-storage-api-simulator-first-app-shells
reviewed: 2026-08-27T19:21:41Z
depth: standard
files_reviewed: 32
files_reviewed_list:
  - assets/mock/letters/carta1.txt
  - assets/mock/letters/carta2.txt
  - assets/mock/letters/carta3.txt
  - assets/mock/photos/foto1.txt
  - assets/mock/photos/foto2.txt
  - assets/mock/photos/foto3.txt
  - assets/mock/tracks/faixa1.txt
  - assets/mock/tracks/faixa2.txt
  - assets/mock/tracks/faixa3.txt
  - lv_conf.h
  - platformio.ini
  - src/board/board.h
  - src/board/native_sdl/board.c
  - src/board/waveshare_s3_35/board.c
  - src/main.c
  - src/ratimos/apps/album_app.c
  - src/ratimos/apps/cartas_app.c
  - src/ratimos/apps/config_app.c
  - src/ratimos/apps/jogos_app.c
  - src/ratimos/apps/musica_app.c
  - src/ratimos/logo_image.c
  - src/ratimos/logo_image.h
  - src/ratimos/row_list.c
  - src/ratimos/splash.c
  - src/ratimos/splash.h
  - src/ratimos/theme.h
  - src/storage/content_api.h
  - src/storage/games.c
  - src/storage/letters.c
  - src/storage/photos.c
  - src/storage/settings.c
  - src/storage/storage.c
  - src/storage/tracks.c
  - test/test_storage/test_storage.c
  - tools/convert_logo.py
findings:
  critical: 0
  warning: 3
  info: 3
  total: 6
status: issues_found
---

# Phase 01: Code Review Report

**Reviewed:** 2026-08-27T19:21:41Z
**Depth:** standard
**Files Reviewed:** 32 (`app_shell.c/.h`, `home_screen.c/.h`, `status_bar.c/.h`, and `theme.c` were additionally pulled in during review because every app file's screen-caching behavior and shared UI chrome depend directly on them, per the task's explicit request to verify the caching fix "consistently across all 5 apps")
**Status:** issues_found

## Summary

This phase implements the RatimOS shell (splash boot sequence, home screen, 5 app screens) and a synchronous, allocation-free Storage/Content API backed by hardcoded mock fixtures. The two previously-fixed bugs called out in the review brief were both verified as correctly and consistently resolved:

1. **Screen-caching fix**: All 5 app screens (`cartas_app.c`, `jogos_app.c`, `musica_app.c`, `album_app.c`, `config_app.c`) — plus `home_screen.c` — use the identical `static lv_obj_t * s_X_screen = NULL; if (!s_X_screen) { s_X_screen = build_X_screen(); } lv_screen_load(s_X_screen);` pattern. No app rebuilds its screen on repeat visits. The fix was applied uniformly, not copy-pasted with drift.
2. **Splash step table**: `src/ratimos/splash.c`'s `s_steps[]` calls `ratimos_storage_mount()` followed by all 5 `index_*()` functions (`index_letters`, `index_photos`, `index_tracks`, `index_games`, `index_settings`) before `ratimos_home_screen_show()` is ever invoked, so every app screen's `list_*()`/`get_settings()` call is guaranteed to read from an already-populated cache by the time a user can reach it.

No buffer overflows, injection vectors, hardcoded secrets, or crashes were found. All string writes into fixed-size buffers go through `snprintf`/bounded `fgets`, and all `list_*()` loops are correctly bounded by `min(cached_count, max_count)`. `src/board/waveshare_s3_35/board.c` is a harmless, side-effect-free stub (empty bodies + a `(void)` cast) with no hazard beyond its documented incompleteness.

The issues found below are all quality/robustness gaps rather than functional bugs: a triplicated fixture-reading helper across the three file-backed storage domains, a settings getter with no "not yet indexed" guard (unlike its list-based siblings, which safely default to count 0), silent unlogged fallback on fixture I/O failure, a stale doc comment, a write-only state variable, and an unpinned library version. None of these block the phase's UAT-level correctness, but several are worth fixing before more domains build on this pattern.

## Warnings

### WR-01: `ratimos_storage_get_settings()` has no "not yet indexed" guard, unlike every other domain

**File:** `src/storage/settings.c:12,22-25`
**Issue:** `s_settings` is a plain file-scope `static ratimos_settings_t`, zero-initialized by the C runtime. Every other domain (`letters`, `photos`, `tracks`, `games`) exposes a `list_*()` getter that naturally signals "not indexed yet" by returning `0` (via `s_*_count`), which every caller already treats as a legitimate empty state (`config_app.c` and friends render an empty-state row). `ratimos_storage_get_settings()` has no equivalent signal: if it is ever called before `ratimos_storage_index_settings()` runs (e.g. a future refactor reorders the splash step table, or a new caller reads settings from outside the splash-then-home flow), it silently returns a struct that *looks* like valid data — `brightness_pct == 0`, `volume_pct == 0`, `firmware_version[0] == '\0'` — with no way for the caller to distinguish "genuinely 0%" from "never indexed." Today's call graph happens to be safe only because the splash step table always runs index_settings before `config_app.c` can be reached, but nothing in the code enforces or asserts that invariant.
**Fix:** Add an explicit "indexed" flag (mirroring the `s_*_count` pattern used elsewhere) and either assert on it in `ratimos_storage_get_settings()` (debug builds) or seed `s_settings` with non-zero, obviously-placeholder defaults so an un-indexed read fails loudly instead of looking plausible:
```c
static ratimos_settings_t s_settings;
static int s_settings_indexed = 0;

void ratimos_storage_index_settings(void)
{
    s_settings.brightness_pct = 80;
    s_settings.volume_pct = 50;
    snprintf(s_settings.firmware_version, sizeof(s_settings.firmware_version), "0.1.0-sim");
    snprintf(s_settings.storage_used_label, sizeof(s_settings.storage_used_label), "12 MB / 512 MB (simulado)");
    s_settings_indexed = 1;
}

ratimos_settings_t ratimos_storage_get_settings(void)
{
    /* fires loudly during development instead of silently returning zeros */
    LV_ASSERT_MSG(s_settings_indexed, "get_settings() called before index_settings()");
    return s_settings;
}
```

### WR-02: Fixture read failures are swallowed with no logging, on top of CWD-fragile hardcoded relative paths

**File:** `src/storage/letters.c:32-36`, `src/storage/photos.c:29-40`, `src/storage/tracks.c:30-41`
**Issue:** All three file-backed domains resolve fixtures via relative paths (`"assets/mock/letters/carta1.txt"`, etc.) hardcoded in `s_letter_files`/`s_photo_files`/`s_track_files`, which only resolve correctly if the process's current working directory is the repo root at run time. If `fopen()` fails for any reason (wrong CWD, missing file, permissions), the code falls back to `"sem titulo"` with **no log, warning, or return code** indicating anything went wrong — the UI renders successfully with a plausible-looking placeholder title, making a genuine fixture/deployment problem indistinguishable from working-as-intended in the field. This directly works against debuggability: a developer (or worse, running on real hardware later against SD-card paths) could ship with all-"sem titulo" screens and have no signal pointing at the root cause.
**Fix:** Log on the failure path (guarded so it compiles out or is a no-op on the eventual embedded target if desired), e.g.:
```c
FILE * f = fopen(s_letter_files[i], "r");
if (!f) {
    LV_LOG_WARN("letters: failed to open fixture '%s' (cwd-relative) - using placeholder title", s_letter_files[i]);
    snprintf(s_letters[i].title, sizeof(s_letters[i].title), "sem titulo");
    continue;
}
```

### WR-03: First-line-title-or-default logic is triplicated with inconsistent structure across the three file-backed domains

**File:** `src/storage/letters.c:38-52` (inlined directly in the loop, no helper extracted), `src/storage/photos.c:27-50` (`static void read_title_or_default(...)`), `src/storage/tracks.c:28-51` (`void ratimos_storage_tracks_read_title_or_default(...)`, non-static, exported for a unit test)
**Issue:** The exact same "open file, `fgets` first line, strip `\r\n`, fall back to `sem titulo` on any failure" logic is implemented three separate times with three different structures: inlined in `letters.c`, a private `static` helper in `photos.c`, and a public (non-`static`) helper in `tracks.c` (kept non-static purely so `test_storage.c` can reach it). Any future fix to this logic (e.g. UTF-8-safe truncation, or the logging suggested in WR-02) now has to be applied — and kept in sync — in three places, and the games domain will be a fourth once it becomes file-backed. The current divergence (inline vs. static vs. exported) is itself a sign this has already started drifting.
**Fix:** Extract a single shared internal helper (e.g. `src/storage/storage_internal.h` + a small `.c`, or a shared `storage_util.c`) and have all three domains call it:
```c
/* storage_internal.h */
void ratimos_storage_read_title_or_default(const char * path, char * out, size_t out_size);
```
Then `letters.c`, `photos.c`, and `tracks.c` each become a thin loop calling this one implementation, and the unit test in `test_storage.c` targets the single shared symbol instead of the tracks-specific one.

## Info

### IN-01: Stale doc comment in `content_api.h` claims only the `letters` domain is implemented

**File:** `src/storage/content_api.h:13-16`
**Issue:** The header comment reads: *"Nesta fase (01-01) apenas o dominio `letters` esta realmente implementado ... os demais tem apenas a assinatura declarada aqui, sem arquivo .c correspondente ainda (job do plano 01-03)."* This was accurate as of plan 01-01 but is now stale — `src/storage/photos.c`, `tracks.c`, `games.c`, and `settings.c` all exist and are fully implemented (confirmed in this review). A future contributor reading this header in isolation would be misled into thinking 4 of 5 domains are still stubs.
**Fix:** Update the comment to reflect current state, e.g. "Todos os 5 dominios estao implementados (ver src/storage/*.c); `games` e o unico que nao e file-backed (ver games.c)."

### IN-02: `s_mounted` in `storage.c` is written but never read anywhere

**File:** `src/storage/storage.c:11,15`
**Issue:** `ratimos_storage_mount()` sets `static int s_mounted = 1;` but no function in the codebase ever reads `s_mounted` — it has no observable effect and no getter exposes it. As written it's dead state that exists only to make the splash progress bar's first tick "look like work happened."
**Fix:** Either wire it into the WR-01 guard pattern above (e.g. have `index_*()` functions assert `s_mounted` before running, giving the variable an actual purpose), or remove it and document `ratimos_storage_mount()` as an intentional no-op placeholder for a future real mount step.

### IN-03: LVGL dependency pinned with a caret range instead of an exact version

**File:** `platformio.ini:19`
**Issue:** `lib_deps = lvgl/lvgl@^9.2.2` allows any `9.x` release `>= 9.2.2` to be resolved on a fresh checkout, including versions newer than the `9.5.x` the project's own stack documentation (CLAUDE.md) calls out as the version this codebase was built/verified against. The project's own guidance elsewhere emphasizes pinning exact release tags "so builds are reproducible across the project's long, no-deadline timeline" (see the `pioarduino` platform note) — the same reasoning applies to LVGL itself, since a minor LVGL update landing mid-project could silently change `lv_conf.h` schema expectations or widget defaults.
**Fix:** Pin an exact version matching what's actually been verified, e.g. `lib_deps = lvgl/lvgl@9.5.0`.

---

_Reviewed: 2026-08-27T19:21:41Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
