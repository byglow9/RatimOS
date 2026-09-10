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
| Heading font, 16px | `src/ratimos/fonts/ratimos_font_title_16.c` | `tools/convert_title_font.sh` (wraps `lv_font_conv@1.5.3`) | *Press Start 2P* by The Press Start 2P Project Authors (Cody "CodeMan38" Boisclair), sourced from the Google Fonts `google/fonts` repository | **OFL** (SIL Open Font License 1.1) — permissive, no attribution obligation beyond retaining the license itself; the source TTF is not committed (`assets/fonts/source/` is gitignored), only the generated bitmap C output |
| Display font, 20px | `src/ratimos/fonts/ratimos_font_title_20.c` | `tools/convert_title_font.sh` (wraps `lv_font_conv@1.5.3`) | Same as above — same TTF, converted at a second size | **OFL** (SIL Open Font License 1.1) |
| Body font (Montserrat) | LVGL bundled font (`lv_font_montserrat_14`) | — (bundled with LVGL, unchanged by this phase) | Julieta Ulanovsky / Google Fonts, bundled with the LVGL library this project already depends on | OFL, pre-existing project dependency, not a new asset introduced by this phase |
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
| Title typography | *Press Start 2P* (OFL), a widely available, generically "8-bit arcade" bitmap typeface converted at 16px/20px specifically for this project's chrome | Unknown/unverified — no font file, font name, or glyph asset was obtained from colombiaOS |
| Icon style | Eleven procedurally generated 32x32 pixel icons drawn from geometric primitives (rectangles, ellipses, lines) in exactly two colors (`RATIMOS_COLOR_TEXT`, `RATIMOS_COLOR_PANEL_ACTIVE`), authored by `tools/generate_icon_art.py` | Unknown/unverified — no icon sprite, sprite sheet, or image file from colombiaOS was viewed, copied, or traced during this phase's implementation |

**How they read differently:** RatimOS's badge is a solid `#e6010f` red circle behind a
white/violet two-tone glyph — a flat, high-contrast, single-brand-color treatment driven
entirely by the six palette tokens above. colombiaOS (per the general "retro handheld with a
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
- The one third-party binary this phase depends on (the Press Start 2P TTF) is never
  committed to the repository — only the generated, human-inspectable bitmap C output is.

## Regeneration

The full visual identity can be rebuilt from source at any time:

```bash
# Icons: regenerate the 11 source PNGs, then recompile the LVGL descriptors
python3 tools/generate_icon_art.py
python3 tools/convert_images.py --manifest assets/icons/manifest.json \
    --out-c src/ratimos/icons.c --out-h src/ratimos/icons.h --guard RATIMOS_ICONS_H

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
