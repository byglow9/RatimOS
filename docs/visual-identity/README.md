# RatimOS visual identity — provenance and distinctness record

This is the checkable artifact behind ROADMAP Phase 02.1 success criterion 1 ("visually
distinct, not copied") — a grep-verifiable record instead of a subjective claim (VISUAL-01).

## Provenance

One row per shipped visual asset: what it is, where its source lives in this repo, which
generator script produced the compiled artifact, its author/origin, and its license. No row
below names the reference project this identity is inspired by but not copied from.

| Asset id | Source file (this repo) | Generator script | Author / origin | license |
|----------|--------------------------|-------------------|------------------|---------|
| Logo | `logo/RatimOS.png` | `tools/convert_logo.py` (`src/ratimos/logo_image.c`/`.h`) | Original artwork brought by the project owner (tower/chess-piece + "RatimOS" gradient, Phase 1 / D-17) | Project-owned, all rights reserved |
| Palette (6 chrome tokens) | `src/ratimos/theme.h` | — (hand-sampled, not generated) | Pixel-sampled directly from `logo/RatimOS.png` (Phase 1 / D-17) | Project-owned, derived from the logo above |
| Palette (7 game-semantic tokens) | `src/ratimos/theme.h` | — (hand-authored) | Chosen by the developer to extend the locked chrome palette with gameplay-feedback colors (plan 02.1-01), never used as chrome | Project-owned |
| Icon `home_jogos` | `assets/icons/home_jogos.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (dice/games motif), project-owned generator script, no external reference | Project-owned |
| Icon `home_musica` | `assets/icons/home_musica.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (music note motif) | Project-owned |
| Icon `home_album` | `assets/icons/home_album.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (photo frame + landscape motif) | Project-owned |
| Icon `home_cartas` | `assets/icons/home_cartas.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (envelope motif) | Project-owned |
| Icon `home_config` | `assets/icons/home_config.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (gear motif) | Project-owned |
| Icon `home_castelo` | `assets/icons/home_castelo.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn, simplified tower silhouette reusing the logo's own tower motif | Project-owned |
| Icon `game_sudoku` | `assets/icons/game_sudoku.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (3x3 grid motif) | Project-owned |
| Icon `game_paciencia` | `assets/icons/game_paciencia.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (playing-card motif) | Project-owned |
| Icon `game_termo` | `assets/icons/game_termo.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (letter-tile motif) | Project-owned |
| Icon `game_cruzadinha` | `assets/icons/game_cruzadinha.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (crossword-grid motif) | Project-owned |
| Icon `game_conexo` | `assets/icons/game_conexo.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (grouped-dots motif) | Project-owned |
| Icon `row_empty` (26x26) | `assets/icons/row_empty.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (empty-tray motif, empty-state list rows), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `row_carta` (26x26) | `assets/icons/row_carta.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (sealed-envelope motif, one letter row), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `row_playlist` (26x26) | `assets/icons/row_playlist.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (track-list + note motif), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `row_track` (26x26) | `assets/icons/row_track.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (play-button motif), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `cfg_brilho` (26x26) | `assets/icons/cfg_brilho.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (sun motif, brightness setting), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `cfg_volume` (26x26) | `assets/icons/cfg_volume.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (speaker + waves motif), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `cfg_firmware` (26x26) | `assets/icons/cfg_firmware.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (chip motif), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `cfg_armazenamento` (26x26) | `assets/icons/cfg_armazenamento.png` | `tools/generate_icon_art.py` + `tools/convert_images.py` | Procedurally drawn (microSD card motif), replaces a raw-letter fallback, plan 02.1-14 | Project-owned |
| Icon `system_rook` (topbar system logo) | `assets/icons/system_rook.png` | `tools/prepare_system_rook.py` + `tools/convert_images.py` | Original artwork provided by the project owner (red rook/pawn with flag; chroma-keyed, alpha-thresholded, quantized to 6 colors and NEAREST-downscaled to 14x20px to fit the 16-color icon palette), plan 02.1-10 | Project-owned |
| Background `bg_dither` | `assets/backgrounds/bg_dither.png` (320x480) | `tools/generate_bg_dither.py` + `tools/convert_bg_dither.py` (`src/ratimos/bg_images.c`/`.h`, RGB565 drawn 1:1 from flash) | Procedurally drawn (4-stop purple -> magenta -> orange gradient, Bayer 4x4 ordered dither quantized to 10 levels on an 80x120 grid, NEAREST to 320x480, 1px scanlines every 3 rows), project-owned generator script, plan 02.1-09; regenerated in plan 02.1-14 to be pixel-identical to the approved sketch asset `.planning/sketches/themes/bg-dither-scan.png` (`--check`) | Project-owned |
| Heading font, 16px | `src/ratimos/fonts/ratimos_font_title_16.c` | `tools/convert_title_font.sh` (wraps `lv_font_conv@1.5.3`) | *Press Start 2P* by The Press Start 2P Project Authors (Cody "CodeMan38" Boisclair), sourced from the Google Fonts `google/fonts` repository | **OFL** (SIL Open Font License 1.1) — permissive, no attribution obligation beyond retaining the license itself; the source TTF is not committed (`assets/fonts/source/` is gitignored), only the generated bitmap C output |
| Display font, 20px | `src/ratimos/fonts/ratimos_font_title_20.c` | `tools/convert_title_font.sh` (wraps `lv_font_conv@1.5.3`) | Same as above — same TTF, converted at a second size | **OFL** (SIL Open Font License 1.1) |
| Row-title font, 8px | `src/ratimos/fonts/ratimos_font_title_8.c` | `tools/convert_title_font.sh` (wraps `lv_font_conv@1.5.3`) | Same *Press Start 2P* TTF, converted at 8px for list-row titles (sketch 003-C), plan 02.1-14 | **OFL** (SIL Open Font License 1.1) |
| Body/chrome font, 10/11/12px | `src/ratimos/fonts/ratimos_font_mono_10.c`, `_11.c`, `_12.c` (12px is `LV_FONT_DEFAULT`) | `tools/convert_title_font.sh <ttf> ratimos_font_mono 10 11 12 --fallback lv_font_montserrat_14` | *JetBrains Mono* Regular by The JetBrains Mono Project Authors, from the official release `https://github.com/JetBrains/JetBrainsMono/releases/download/v2.304/JetBrainsMono-2.304.zip` (`fonts/ttf/JetBrainsMono-Regular.ttf`), plan 02.1-14. Like Press Start 2P, the TTF and its `OFL.txt` stay in the gitignored `assets/fonts/source/`; only the generated C is committed | **OFL** (SIL Open Font License 1.1) |
| Symbol fallback font (Montserrat 14) | LVGL bundled font (`lv_font_montserrat_14`) | — (bundled with LVGL) | Julieta Ulanovsky / Google Fonts, bundled with LVGL. Since plan 02.1-14 it is no longer the body font; it is only the `.fallback` of the mono fonts, so `LV_SYMBOL_*` glyphs (e.g. the bottombar "voltar" arrow) still render | OFL, pre-existing project dependency |
| Progression stage `castle_stage_00_terreno_vazio` | `assets/progress/castle_stage_00_terreno_vazio.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (bare walled grounds, no structure), plan 02.1-03 | Project-owned |
| Progression stage `castle_stage_01_alicerce` | `assets/progress/castle_stage_01_alicerce.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (foundation stones laid) | Project-owned |
| Progression stage `castle_stage_02_muros_canteiro` | `assets/progress/castle_stage_02_muros_canteiro.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (walls raised, first garden bed) | Project-owned |
| Progression stage `castle_stage_03_torres_florindo` | `assets/progress/castle_stage_03_torres_florindo.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (towers raised, first blooms) | Project-owned |
| Progression stage `castle_stage_04_bandeira_jardim_cheio` | `assets/progress/castle_stage_04_bandeira_jardim_cheio.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (flag flying, full garden) | Project-owned |
| Progression stage `castle_stage_05_completo` | `assets/progress/castle_stage_05_completo.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (complete castle, fullest garden) | Project-owned |
| Unlock `unlock_sudoku_roseira` | `assets/progress/unlock_sudoku_roseira.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (rose bush motif), sudoku's exclusive unlock (D-05) | Project-owned |
| Unlock `unlock_paciencia_bandeira` | `assets/progress/unlock_paciencia_bandeira.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (pennant motif), paciência's exclusive unlock (D-05) | Project-owned |
| Unlock `unlock_termo_arvore` | `assets/progress/unlock_termo_arvore.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (small tree motif), termo's exclusive unlock (D-05) | Project-owned |
| Unlock `unlock_cruzadinha_fonte` | `assets/progress/unlock_cruzadinha_fonte.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (fountain motif), cruzadinha's exclusive unlock (D-05) | Project-owned |
| Unlock `unlock_conexo_portao` | `assets/progress/unlock_conexo_portao.png` | `tools/generate_progress_art.py` + `tools/convert_images.py` | Procedurally drawn (arched gate motif), conexo's exclusive unlock (D-05) | Project-owned |
| Word list (termo/dueto/quarteto) | `assets/wordlists/termo_answers.txt` (577 words) + `assets/wordlists/termo_accepted.txt` (6,011 words) | `tools/curate_termo_words.py` (`src/ratimos/apps/jogos/termo_words.c`/`.h`) | Curated subset of the `fserb/pt-br` Brazilian Portuguese lexicon (https://github.com/fserb/pt-br), filtered to 5-letter words, ranked by the source's bundled ICF frequency score, and additionally passed through a manual suitability review (23 entries removed — proper nouns and morbid/religious terms; see `assets/wordlists/README.md`), plan 02.1-06 | **MIT** (verified against the source repository's `LICENSE` file) |
| Crossword content (cruzadinha) | `assets/crosswords/wordlists.json` (5 puzzles, 44 answer+clue pairs) | `tools/generate_crossword.py` (`src/ratimos/apps/jogos/cruzadinha_puzzles.c`/`.h`) | Grid layout (word placement, intersection math, clue numbering) is generated by a backtracking/CSP algorithm — language-agnostic, no third-party content involved. All 5 puzzles' answers and clue text in this initial set are project-authored from scratch by the developer (plan 02.1-07); no network access was available in this execution session to fetch the optional **Dicionário-Aberto** dataset (`ioxua/dicionario-aberto`, **CC BY-SA 2.5 PT**), so no clue in this set is derived from it. D-12 explicitly accepts Dicionário-Aberto as an optional future seed source for additional puzzles' clue wording — if a future contribution derives clue text from it, that clue's derivation must be recorded (per D-12's share-alike obligation) alongside the puzzle it seeds, in `assets/crosswords/README.md` | Project-owned (all 5 puzzles' content); Dicionário-Aberto itself is **CC BY-SA 2.5 PT** and remains unused-but-accepted for future puzzles |

## Distinctness vs colombiaOS

RatimOS's overall look-and-feel takes structural/menu-navigation inspiration from a
third-party reference project the developer researched before starting RatimOS, known as
**colombiaOS** (per `.planning/PROJECT.md`'s own wording — it is never named as a source of
any asset in the Provenance table above, and is named here exactly once, in this section
only, precisely so that fact is checkable rather than asserted). This section makes the
distinction between "inspired by" and "copied from" checkable side by side:

| Dimension | RatimOS | colombiaOS |
|-----------|---------|------------|
| Palette | 6 locked hex values sampled from `logo/RatimOS.png`: `#000000` bg, `#2a123f`/`#4e2277` violet panels, `#e6010f` red accent, `#f5f2f8`/`#a997ba` text (Phase 1 / D-17) | Not sampled, not measured, not reused — RatimOS never inspected or extracted color values from colombiaOS's UI |
| Title typography | *Press Start 2P* (OFL), a widely available, generically "8-bit arcade" bitmap typeface converted at 8px/16px/20px for this project's chrome; body/chrome text in *JetBrains Mono* (OFL) since plan 02.1-14 | Unknown/unverified — no font file, font name, or glyph asset was obtained from colombiaOS |
| Icon style | Procedurally generated pixel icons (32x32 launcher icons, 26x26 list-row icons since plan 02.1-14) drawn from geometric primitives (rectangles, ellipses, lines) in exactly two colors (`RATIMOS_COLOR_TEXT`, `RATIMOS_COLOR_PANEL_ACTIVE`), authored by `tools/generate_icon_art.py`, rendered as bare pixel-art images with no badge, circle, or container behind them (plan 02.1-09); plus the owner-supplied `system_rook` logo in the topbar | Unknown/unverified — no icon sprite, sprite sheet, or image file from colombiaOS was viewed, copied, or traced during this phase's implementation |
| Surfaces / background | Pre-rendered dithered purple -> magenta -> orange gradient bitmap behind every screen; square-cornered translucent bevel cards with a dark 2px border; file-explorer breadcrumb sectionbar (`./home/jogos/conexo`) | Unknown/unverified — no background, card, or header asset was obtained from colombiaOS |

**How they read differently:** RatimOS's icons are bare two-tone (white/violet) pixel-art
glyphs sitting directly on square-cornered translucent bevel cards over a dithered gradient
background — no badge, circle, or other container behind them (the earlier solid red circle
badge was removed in plan 02.1-09). The red `#e6010f` accent survives only as a highlight
color (brand wordmark, breadcrumb current segment, CTAs), and the system is identified by
the owner's own red rook logo. colombiaOS (per the general "retro handheld with a
d-pad" concept description in `.planning/PROJECT.md`) served as a structural/menu-navigation
inspiration, not a visual asset source: nothing in this repository's icon pixels, font
glyphs, or hex values was extracted from it. The palette itself was independently sampled
from the project's own commissioned logo artwork in Phase 1 (D-17), not borrowed from any
other source, and every icon in this phase was drawn from scratch by a project-owned script
rather than traced from an existing reference image.

## No copied assets

Verification performed for this record:

- `grep -ric 'colombiaos' src/ tools/ assets/` (the actual name of the reference project, kept
  out of source/tooling/asset paths entirely, appearing only in this distinctness section by
  loose description) returns zero matches across every source, tooling, and asset directory
  this phase touched.
- Every binary visual asset this phase ships traces to a committed source file plus a
  committed generator script: the 11 icon PNGs to `tools/generate_icon_art.py`, the compiled
  icon descriptors to `tools/convert_images.py`, and the two pixel fonts to
  `tools/convert_title_font.sh`. Nothing is a hand-pasted or manually-edited binary blob.
- Gap-closure round (plans 02.1-09 to 02.1-12): the dithered background traces to
  `tools/generate_bg_dither.py` + `tools/convert_bg_dither.py`, and the `system_rook` logo
  traces to the owner-supplied reference image via `tools/prepare_system_rook.py` +
  `tools/convert_images.py`. The `grep -ric 'colombiaos' src/ tools/ assets/` check above
  was re-run after these files landed (plan 02.1-12) and still returns zero matches.
- The one third-party binary this phase depends on (the Press Start 2P TTF) is never
  committed to the repository — only the generated, human-inspectable bitmap C output is.

## Regeneration

The full visual identity can be rebuilt from source at any time:

```bash
# Icons: regenerate the 11 source PNGs, then recompile the LVGL descriptors
python3 tools/generate_icon_art.py
python3 tools/convert_images.py --manifest assets/icons/manifest.json \
    --out-c src/ratimos/icons.c --out-h src/ratimos/icons.h --guard RATIMOS_ICONS_H

# System logo: re-process the owner-supplied rook reference into the 14x20px icon
# source (then rerun the icon convert_images.py command above -- it is the 12th
# manifest entry) (plan 02.1-10)
python3 tools/prepare_system_rook.py

# Background: regenerate the dithered gradient PNG, then compile it to RGB565
# (src/ratimos/bg_images.c/.h) (plan 02.1-09)
python3 tools/generate_bg_dither.py
python3 tools/convert_bg_dither.py

# Title font: download Press Start 2P (OFL) into the gitignored assets/fonts/source/
# directory, then convert both sizes
tools/convert_title_font.sh assets/fonts/source/PressStart2P-Regular.ttf

# Progression (castelo/jardim) art: regenerate the 11 source PNGs, then recompile
# the LVGL descriptors -- same convert_images.py pipeline as the icons above,
# reused unmodified (plan 02.1-03)
python3 tools/generate_progress_art.py
python3 tools/convert_images.py --manifest assets/progress/manifest.json \
    --out-c src/ratimos/progress_images.c --out-h src/ratimos/progress_images.h \
    --guard RATIMOS_PROGRESS_IMAGES_H

# Termo/Dueto/Quarteto word list: recompile the C arrays from the already-curated
# assets/wordlists/*.txt files (safe, does not touch the manually-reviewed content) --
# see assets/wordlists/README.md for the --regenerate flag, which rebuilds the .txt
# files from a fresh fserb/pt-br checkout and REQUIRES repeating the manual
# suitability pass documented there (plan 02.1-06)
python3 tools/curate_termo_words.py --source <path-to-fserb-pt-br-checkout> \
    --out-c src/ratimos/apps/jogos/termo_words.c \
    --out-h src/ratimos/apps/jogos/termo_words.h
```

Both pipelines are deterministic — running either command twice in a row produces
byte-identical generated output.
