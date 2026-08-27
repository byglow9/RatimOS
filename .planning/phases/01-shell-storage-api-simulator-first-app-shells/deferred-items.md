# Phase 1 — Deferred Items

Out-of-scope discoveries surfaced during execution, logged (not fixed) per the executor's scope-boundary rule.

## Screen-retention leak in jogos/musica/album/config app screens

**Discovered during:** 01-01 Task 1 checkpoint verification (crash repro + fix — see `01-01-SUMMARY.md` Deviations section).

**What:** `ratimos_jogos_show()`, `ratimos_musica_show()`, `ratimos_album_show()`, and `ratimos_config_show()` each build a brand-new `lv_obj_t` screen via `ratimos_app_shell_create()` on every visit, with no caching (unlike `ratimos_home_screen_show()`'s build-once pattern) and no `lv_obj_delete()` of the previous screen. Repeated navigation to any one of these screens leaks memory unboundedly.

**Why not fixed now:** None of these 4 files are in 01-01's `files_modified` list (only `cartas_app.c` is, which — was fixed this task after checkpoint verification caught a live crash). Fixing all 4 now would be scope creep beyond the current task/plan.

**Confirmed mechanism (reproduced against `cartas_app.c` before its fix):** with LVGL's builtin allocator (`LV_MEM_SIZE`, previously unset -> 64KB default), ~3-4 repeated visits to a single unclamped app screen exhausts the heap (`lv_realloc: couldn't reallocate memory` / `lv_array_resize` assert). `lv_conf.h` now sets `LV_MEM_SIZE (512 * 1024U)` for `native_sim` as headroom, but that only buys more visits before the same failure mode recurs for these 4 files — it does not eliminate the leak.

**Recommendation:** Apply the same cache-once pattern already used by `ratimos_home_screen_show()` and (as of the 01-01 checkpoint fix) `ratimos_cartas_show()` to `jogos_app.c`, `musica_app.c`, `album_app.c`, and `config_app.c` when those files are next touched (plan 01-03 wires musica/album/jogos/config to the Storage API's remaining domains — a natural point to apply this fix at the same time). Flag for the phase's `/gsd-verify-work` or a `/gsd-code-review` pass if not folded into 01-03.

**Severity:** Low for the simulator (PC has abundant RAM even with the leak); becomes real on ESP32-S3 hardware (Phase 3+) where RAM is genuinely constrained — must be resolved (or at minimum re-measured against the real hardware's heap budget) before Phase 3 hardware bring-up locks in a production `lv_conf.h`.
