# Phase 02.1 -- deferred / out-of-scope findings

Logged by plan 02.1-12's final walk (2026-10-05). The walk drove real production
screens through a headless LVGL pointer and dumped frames. Nothing here was caused by
02.1-12's changes, and none of it was fixed in that plan.

## 1. LVGL heap exhaustion crash after visiting most game screens in one session (major) -- RESOLVED in 02.1-15

> Fixed by plan 02.1-15: delete-on-navigate (`ratimos_screen_load()`, only the home stays
> cached) and a cruzadinha screen of 16KB instead of 170KB. Guarded by
> `test/test_navigation_memory` (3-lap session, heap stable at 21984 bytes at home).

- **Symptom:** native_sim SIGSEGV in `lv_text_get_size_attributes()` (label text == NULL),
  preceded by `get_local_style: couldn't allocate local style`.
- **Repro (headless walk):** home -> jogos -> sudoku -> home -> jogos -> paciencia -> home ->
  jogos -> termo (switch to quarteto and back) -> home -> jogos -> cruzadinha => crash while
  building the cruzadinha screen.
- **Cause:** every app/game screen is built once and cached forever (no delete-on-navigate),
  and `LV_MEM_SIZE` is 512KB (`lv_conf.h` already documents this as a known leak to fix
  before hardware). Measured heap use: home+jogos ~8%; cruzadinha alone +34% (~194KB);
  conexo +6%; each top-level app ~+2-3%; castelo ~+2%.
- **Why it matters:** a normal "play every game once" session can crash the simulator,
  and the ESP32-S3 heap will be tighter than this.
- **Suggested fix (separate plan):** delete-on-navigate (or an LRU of 1-2 cached game screens),
  and look at why cruzadinha's screen costs ~194KB.

## 2. Jogos list rows oversized; icon detached from its text (cosmetic / UX)

- Each jogos row is ~150px tall (2.5 rows visible). The title/subtitle sit at the top and the
  icon is vertically centered far below them. This matches the "espacos gigantescos" part of
  the original G-02.1-1 complaint, and it is still present even though first-tap navigation
  now works.

## 3. Fallback glyph text instead of icons on top-level app rows (cosmetic)

- musica / galeria / cartas empty-state rows show a bare `!`, and config rows show `B` / `V`.
  This is `ratimos_badge_create()`'s raw-text fallback for icon ids that have no compiled icon.

## 4. Breadcrumb current segment contrast (cosmetic, needs human eye)

- The leaf segment is `RATIMOS_COLOR_ACCENT` (#e6010f) and is hard to read where the
  sectionbar sits over the magenta part of the dithered gradient.

## 5. Button/keypad layout issues (left for plan 02.1-13, button styling)

- The cruzadinha "proxima palavra" button text is clipped ("oxima palav").
- The sudoku number keypad row is cramped ("9apagar" runs together).
- Conexo's action pills (embaralhar / enviar / novo jogo) still use rounded corners.

## 6. Termo and sudoku are now the heaviest screens (ESP32 sizing, logged by 02.1-15)

- After 02.1-15, per-screen LVGL heap cost (test_navigation_memory, "screen cost"): termo
  138936 bytes (4 boards x 9 rows x 5 tiles = 180 tile objs + 180 labels, all built for
  quarteto even in termo mode), sudoku 78392 (81 cells + 81 labels), everything else <= 32KB.
- Session peak is now 182952 bytes (home + jogos list + termo, briefly, while the old screen
  is being replaced). Not a crash risk at 512KB, but it sets the ESP32-S3 LV_MEM_SIZE floor.
- Same fix as the cruzadinha grid would apply (draw the boards in one object) if the
  hardware heap turns out tight; out of 02.1-15's scope (plan targeted cruzadinha only).
