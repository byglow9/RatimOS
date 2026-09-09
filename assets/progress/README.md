# RatimOS progression (castelo/jardim) source art

Eleven project-authored pixel-art PNGs backing PROGRESSAO-01's garden-inside-castle reward
system (D-03/D-04/D-05/D-06a): six 288x180 stage scenes plus five 32x32 per-game exclusive
unlock sprites. Every id here matches a string in `src/ratimos/progression.c`'s manifest
character for character — that string is the runtime key the castelo screen resolves against
`ratimos_progress_image_by_id()`, never a compiled pointer or a switch/case.

## Authorship

Like the icon set (`assets/icons/README.md`), these PNGs are **not** hand-drawn in an external
editor. They are generated procedurally by `tools/generate_progress_art.py`, a project-owned
Pillow script drawing every scene from plain geometric primitives (rectangles, ellipses, lines,
polygons). No external reference image, sprite sheet, or third-party asset was consulted or
traced to produce this art.

## Palette

The six stage scenes are drawn on a solid `#000000` background (`RATIMOS_COLOR_BG`) using the
same locked chrome palette the icon set uses (`RATIMOS_COLOR_PANEL` `#2a123f`,
`RATIMOS_COLOR_PANEL_ACTIVE` `#4e2277`, `RATIMOS_COLOR_ACCENT` `#e6010f`, `RATIMOS_COLOR_TEXT`
`#f5f2f8`) plus a small, garden-only extension — two greens and a dirt/trunk brown — that is
never used as chrome, CTA, or brand color, exactly like `theme.h`'s existing game-semantic color
extension for the 5 games. The five unlock sprites reuse the same palette at 32x32, matching the
icon set's treatment.

## The six stages: one continuous scene, not six separate pictures

Each stage image **adds to** the previous one rather than replacing it — the same plot of
ground, progressively built up: bare walled grounds → foundation stones laid → walls up with the
first garden bed turned → towers raised with the first blooms → flag flying over a full garden →
complete castle with the garden at its fullest. From stage 02 onward every scene shows both
castle and garden elements in the same frame (D-03's whole point — a garden *inside* the castle
grounds, not two competing visuals).

| id | what's new this stage |
|----|------------------------|
| `castle_stage_00_terreno_vazio` | bare walled grounds — outline only, no structure, no garden |
| `castle_stage_01_alicerce` | foundation stones laid along the base |
| `castle_stage_02_muros_canteiro` | walls raised, first garden bed with a few sprouts |
| `castle_stage_03_torres_florindo` | two corner towers, two garden beds in bloom |
| `castle_stage_04_bandeira_jardim_cheio` | central keep with a flag, three garden beds |
| `castle_stage_05_completo` | full castle (crenellated central keep), one continuous garden bed along the whole base |

## The five unlock sprites

| id | game | motif |
|----|------|-------|
| `unlock_sudoku_roseira` | sudoku | rose bush |
| `unlock_paciencia_bandeira` | paciência | pennant on a pole |
| `unlock_termo_arvore` | termo | small tree |
| `unlock_cruzadinha_fonte` | cruzadinha | garden fountain |
| `unlock_conexo_portao` | conexo | arched gate |

## Regenerating

```bash
python3 tools/generate_progress_art.py
python3 tools/convert_images.py --manifest assets/progress/manifest.json \
    --out-c src/ratimos/progress_images.c --out-h src/ratimos/progress_images.h \
    --guard RATIMOS_PROGRESS_IMAGES_H
```

The first command rewrites `assets/progress/*.png` from `tools/generate_progress_art.py`'s
drawing functions. The second command reuses the exact same manifest-driven conversion pipeline
the icon set uses (unmodified — `tools/convert_images.py` was written in plan 02.1-02
specifically to be reusable for this art) to regenerate `src/ratimos/progress_images.c`/`.h`.
Both commands are deterministic — running them twice in a row produces byte-identical output.

See `docs/visual-identity/README.md` for the full asset provenance table and the distinctness
record against the reference project.
