# Phase 02.1 -- deferred / out-of-scope findings

Logged by plan 02.1-12's final walk (2026-10-05). The walk drove real production
screens through a headless LVGL pointer and dumped frames. Nothing here was caused by
02.1-12's changes, and none of it was fixed in that plan.

## 1. LVGL heap exhaustion crash after visiting most game screens in one session (major)

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
