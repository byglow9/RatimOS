# RatimOS icon source art

Eleven 32x32 pixel-art PNGs, project-authored source art for VISUAL-01 (D-07).

## Authorship

These PNGs are **not** hand-drawn in an external pixel-art editor (Piskel/Aseprite, the
production method the UI-SPEC floated as one option). They are generated procedurally by
`tools/generate_icon_art.py`, a project-owned Pillow script that draws each icon with plain
geometric primitives (rectangles, ellipses, lines, polygons) using only two colors from the
already-locked RatimOS palette (`RATIMOS_COLOR_TEXT` `#f5f2f8` and `RATIMOS_COLOR_PANEL_ACTIVE`
`#4e2277`, both defined in `src/ratimos/theme.h`) plus a transparent background. No external
reference image, sprite sheet, or third-party asset was consulted or traced to produce this art.

This substitution (script instead of an external editor) is a production-method detail, not a
scope change: D-07 explicitly left the exact production method open, and the result satisfies
every requirement the UI-SPEC's own recommendation was chasing — project-authored, palette-
consistent, reproducible from source, and free of any dependency on the third-party reference
project named in `docs/visual-identity/README.md`.

## Regenerating

```bash
python3 tools/generate_icon_art.py
python3 tools/convert_images.py --manifest assets/icons/manifest.json \
    --out-c src/ratimos/icons.c --out-h src/ratimos/icons.h --guard RATIMOS_ICONS_H
```

The first command rewrites `assets/icons/*.png` from `tools/generate_icon_art.py`'s drawing
functions. The second command re-runs the manifest-driven conversion pipeline (shared with the
future progression/castle art in plan 02.1-03) to regenerate `src/ratimos/icons.c`/`.h`. Both
commands are deterministic — running them twice in a row produces byte-identical output.

## Icon set

| id | motif |
|----|-------|
| `home_jogos` | five-pip die (generic games) |
| `home_musica` | music note |
| `home_album` | photo frame with a mountain/sun scene |
| `home_cartas` | envelope |
| `home_config` | gear |
| `home_castelo` | simplified castle tower (reuses the logo's tower motif) |
| `game_sudoku` | 3x3 grid with filled cells |
| `game_paciencia` | playing card with a diamond pip |
| `game_termo` | letter tile |
| `game_cruzadinha` | crossword grid with blocked cells |
| `game_conexo` | four dots joined into two pairs |

See `docs/visual-identity/README.md` for the full asset provenance table and the
distinctness record against the reference project.
