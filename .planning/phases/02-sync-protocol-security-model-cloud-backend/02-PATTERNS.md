# Phase 2: Sync Protocol, Security Model & Cloud Backend - Pattern Map

**Mapped:** 2026-08-27
**Files analyzed:** 11 (new) + 1 (modified)
**Analogs found:** 5 / 11 (structural analogs only — this phase is mostly greenfield; no Supabase/Deno/TypeScript/libcurl/cJSON code exists anywhere in this repo)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|--------------------|------|-----------|-----------------|----------------|
| `src/sync/http/http_client.h` | interface/HAL contract | request-response | `src/board/board.h` | role-match (HAL contract style) |
| `src/sync/http/native_curl/http_client.c` | HAL implementation (native) | request-response | `src/board/native_sdl/board.c` | role-match (one-implementation-per-env pattern) |
| `src/sync/http/esp32/http_client.c` (optional stub, Open Question 1) | HAL implementation (stub, excluded from link) | request-response | `src/board/waveshare_s3_35/board.c` | role-match (unbuilt hardware stub pattern) |
| `src/sync/sync_client.h` | service/public API contract | request-response | `src/storage/content_api.h` | role-match (contract-first header, fixed-size structs, no dynamic alloc) |
| `src/sync/sync_client.c` | service (protocol client) | request-response | `src/storage/storage.c` | partial-match (thin C module, single responsibility) — no direct CRUD analog exists |
| `platformio.ini` (modified) | config | build | `platformio.ini` (itself, existing) | exact (edit in place — add `-lcurl` build_flags, cJSON `lib_deps`, keep `build_src_filter` exclusion convention) |
| `test/test_sync/test_sync_unit.c` | test (unit) | transform | `test/test_storage/test_storage.c` | exact (Unity native test scaffold, same file layout convention) |
| `test/test_sync/test_sync_integration.c` | test (integration) | request-response | `test/test_storage/test_storage.c` | role-match (Unity scaffold, but no network-dependent test exists yet to mirror) |
| `supabase/config.toml` | config | — | none | no analog — first Supabase config file in repo |
| `supabase/functions/register-device/index.ts` | controller (Edge Function, event-driven HTTP handler) | request-response | none | no analog — first TypeScript/Deno file in repo |
| `supabase/functions/whats-new/index.ts` | controller (Edge Function, event-driven HTTP handler) | request-response | none | no analog — first TypeScript/Deno file in repo |
| `supabase/migrations/<timestamp>_phase2_schema.sql` | migration | CRUD (schema) | none | no analog — first SQL/migration file in repo |

## Pattern Assignments

### `src/sync/http/http_client.h` (interface, request-response)

**Analog:** `src/board/board.h`

**Structure pattern** (full file, `src/board/board.h` lines 1-25):
```c
#ifndef RATIMOS_BOARD_H
#define RATIMOS_BOARD_H

#include <stdint.h>

/*
 * Contrato de board/HAL do RatimOS (D-05).
 *
 * Cada variante de placa (native_sdl hoje, waveshare_s3_35 na Fase 3)
 * implementa exatamente estas 3 funcoes. `main.c` so conhece este contrato,
 * nunca os detalhes de SDL2/ESP-IDF por baixo.
 */

/* Inicializa o display (janela SDL2 hoje, painel ST7796 na Fase 3). */
void board_display_init(void);
...
#endif
```

**What to copy:**
- Header guard naming convention: `RATIMOS_<MODULE>_H` → use `RATIMOS_SYNC_HTTP_CLIENT_H` (already used verbatim in RESEARCH.md's Pattern 1 example — keep it).
- The doc-comment block at the top of the header explicitly naming which decision (D-xx) drove the contract and which concrete implementation exists today vs. later ("native_curl hoje, esp32 na Fase 7") — mirror this exact "today vs. later phase" annotation style, in Portuguese, matching the rest of the codebase's comment language.
- One-function-per-concern declarations with a one-line doc comment above each, no more.
- Fixed-size struct output (no malloc) — matches `ratimos_http_response_t.body[4096]` already drafted in RESEARCH.md Pattern 1; consistent with `content_api.h`'s no-dynamic-allocation contract below.

---

### `src/sync/http/native_curl/http_client.c` (HAL implementation, request-response)

**Analog:** `src/board/native_sdl/board.c`

**Full file** (`src/board/native_sdl/board.c`, lines 1-28):
```c
/*
 * Board/HAL — implementacao native_sdl (simulador de PC via SDL2).
 * Todo o codigo SDL2 do projeto vive exclusivamente aqui (extraido de
 * src/main.c) — nenhum outro arquivo deve incluir <SDL2/SDL.h>.
 */
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "../board.h"
#include "../../ratimos/theme.h"

void board_display_init(void)
{
    lv_display_t * disp = lv_sdl_window_create(RATIMOS_SCREEN_W, RATIMOS_SCREEN_H);
    (void) disp;
    lv_tick_set_cb(SDL_GetTicks);
}

void board_input_init(void)
{
    lv_indev_t * mouse = lv_sdl_mouse_create();
    (void) mouse;
}

void board_tick(uint32_t idle_time_ms)
{
    SDL_Delay(idle_time_ms > 0 ? idle_time_ms : 5);
}
```

**What to copy:**
- Top-of-file comment states "all vendor-library code lives exclusively here — no other file should include `<curl/curl.h>`" (mirror the exact isolation-boundary pattern: `<SDL2/SDL.h>` → `<curl/curl.h>`, `<cjson/cJSON.h>` stays in `sync_client.c` instead since JSON parsing is a separate concern from the HTTP transport per the RESEARCH.md structure).
- Relative include of the parent contract header (`"../board.h"` → `"../http_client.h"`).
- Each function is a thin, direct wrapper around the vendor library with no extra abstraction layers — same minimalism level as `board_tick`'s one-line `SDL_Delay` call.
- Directory nesting convention: `src/board/<variant>/board.c` → `src/sync/http/<variant>/http_client.c` (already matches RESEARCH.md's proposed structure — confirmed consistent with existing repo convention, not just a research proposal).
- Concrete libcurl implementation itself: use RESEARCH.md's Pattern 1 code example verbatim (already cites `curl.se/libcurl/c/httpcustomheader.html`) — this is a first-party greenfield implementation, no local analog exists for the libcurl call sequence itself.

---

### `src/sync/http/esp32/http_client.c` (optional stub — Open Question 1)

**Analog:** `src/board/waveshare_s3_35/board.c`

Not read in full (out of scope for this phase's link — excluded via `build_src_filter`), but its existence and exclusion pattern is the direct analog:
```ini
build_src_filter =
    +<*>
    -<board/waveshare_s3_35/*>
```
**What to copy:** if the planner follows RESEARCH.md's Open Question 1 recommendation, mirror this exact `build_src_filter` exclusion line for the new `esp32/` HTTP implementation: add `-<sync/http/esp32/*>` to `native_sim`'s `build_src_filter` in `platformio.ini`, matching the existing `-<board/waveshare_s3_35/*>` line style and placement.

---

### `src/sync/sync_client.h` (service contract, request-response)

**Analog:** `src/storage/content_api.h`

**Full file excerpt** (`src/storage/content_api.h`, lines 1-67 — struct/API shape):
```c
#ifndef RATIMOS_STORAGE_CONTENT_API_H
#define RATIMOS_STORAGE_CONTENT_API_H

#include <stddef.h>

/*
 * Storage/Content API do RatimOS (D-09/D-10/D-11).
 *
 * Contrato unico, sincrono e sem alocacao dinamica, compartilhado pelos 5
 * dominios de conteudo do dispositivo. Cada app le exclusivamente atraves
 * destas funcoes — nunca faz I/O de arquivo diretamente.
 * ...
 */

typedef struct {
    char id[16];
    char title[64];
} ratimos_letter_t;
...
size_t ratimos_storage_list_letters(ratimos_letter_t * out, size_t max_count);
```

**What to copy:**
- Doc comment referencing the driving decision IDs (`D-09/D-10/D-11`) — for `sync_client.h`, reference `D-10` (metadata-only content_items shape) and cite the RESEARCH.md struct fields (`title`, `type`, `content_date`, `url`).
- Fixed-size `char[N]` struct fields, no pointers/dynamic allocation — same convention as `ratimos_letter_t`/`ratimos_photo_t`. Apply this to a new `ratimos_content_item_t` struct (title, type, date, url as fixed buffers).
- Getter signature convention: `size_t ratimos_storage_list_X(X_t * out, size_t max_count)` returning count copied, capped at `max_count` → mirror as `size_t ratimos_sync_whats_new(ratimos_content_item_t * out, size_t max_count, int * out_http_status)` (or similar) so `src/sync/` composes with existing consumer code the same way storage does.
- "Never touches disk/network on the getter, only on the explicit fetch/index call" pattern — `content_api.h`'s index-then-get split maps to sync's "explicit HTTPS call function," since sync has no persistent cache to index into (out of scope, that's Phase 7/storage's job) — keep sync_client's function synchronous and single-purpose (one call = one network round trip), not a cache-then-read split.

---

### `src/sync/sync_client.c` (service/protocol client, request-response)

**Analog:** `src/storage/storage.c` (thin single-purpose module) — partial match only, no true CRUD/JSON-parsing analog exists in this repo.

**Full file** (`src/storage/storage.c`, lines 1-17):
```c
/*
 * Storage/Content API — mount do backend (D-03).
 *
 * Backend nativo/mock desta fase nao precisa de nenhuma montagem real de
 * filesystem (le fixtures via fopen() direto, ver letters.c) — esta funcao
 * so marca "a camada de storage esta pronta", representando o primeiro
 * passo de progresso real do splash.
 */
#include "content_api.h"

static int s_mounted = 0;

void ratimos_storage_mount(void)
{
    s_mounted = 1;
}
```

**What to copy:**
- File-level doc comment citing the driving decision and stating in one sentence what this file's single responsibility is (not a multi-paragraph essay) — apply the same brevity to `sync_client.c`'s top comment (cite D-09/D-10, state "calls http_client.h's contract, parses the JSON body via cJSON, fills the caller's fixed-size buffer — never touches curl/cJSON types outside this file").
- Static module-level state guarded by a private flag (`static int s_mounted`) — if `sync_client.c` needs any client-side state (e.g., a cached last-fetch timestamp), follow this same `static` file-scope, no-globals-exposed convention.
- No direct analog exists for the actual JSON-parsing-then-struct-fill logic (letters.c/photos.c would be closer but were not required reading here); the concrete cJSON parsing code should follow RESEARCH.md's Pattern 1/`whats-new` response shape (`{"items": [...]}`) directly, since there is no existing JSON-consuming C code in this repo to pattern-match against.

---

### `platformio.ini` (config, modified in place)

**Analog:** itself (existing file, edit don't replace)

**Current relevant block** (`platformio.ini`, `[env:native_sim]`):
```ini
[env:native_sim]
platform = native
build_flags =
    -std=gnu11
    -I.
    -Isrc
    -DLV_CONF_INCLUDE_SIMPLE
    -lSDL2
lib_deps =
    lvgl/lvgl@^9.2.2
lib_ldf_mode = deep+
build_src_filter =
    +<*>
    -<board/waveshare_s3_35/*>
test_build_src = yes
```

**What to copy/extend:**
- Append `-lcurl` to the existing `build_flags` list (same list style, one flag per line).
- Add cJSON to `lib_deps` pinned to an exact tag, matching the existing `lvgl/lvgl@^9.2.2` pinning style: `https://github.com/DaveGamble/cJSON.git#v1.7.19`.
- If the esp32 HTTP stub (Open Question 1) is added, extend `build_src_filter` with a new exclusion line following the exact existing `-<board/waveshare_s3_35/*>` pattern.
- Do not touch `-std=gnu11` — confirmed by RESEARCH.md Pitfall 1 that `src/sync/*.c` must stay compilable as plain C (this is why cJSON, not ArduinoJson, was chosen).

---

### `test/test_sync/test_sync_unit.c` and `test/test_sync/test_sync_integration.c` (test, transform / request-response)

**Analog:** `test/test_storage/test_storage.c`

**Full file structure pattern** (`test/test_storage/test_storage.c`, lines 1-112):
```c
#include <unity.h>
#include "storage/content_api.h"

void setUp(void) {}
void tearDown(void) {}

void test_letters_list_returns_fixture_titles(void)
{
    ratimos_storage_mount();
    ratimos_storage_index_letters();

    ratimos_letter_t out[4];
    size_t n = ratimos_storage_list_letters(out, 4);

    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_STRING("Carta de teste 1", out[0].title);
}
...
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_letters_list_returns_fixture_titles);
    ...
    return UNITY_END();
}
```

**What to copy:**
- `#include <unity.h>` + one project header, `setUp`/`tearDown` no-ops, one `test_*` function per behavior, explicit `RUN_TEST(...)` list in `main()`, `UNITY_BEGIN()`/`UNITY_END()` bookends — apply verbatim to both new test files.
- Top-of-file doc comment explaining what this suite covers and what it depends on (mirrors the existing comment block at lines 1-8) — for `test_sync_integration.c`, this comment MUST document the precondition (a `.env`-provided pre-registered test device token) per RESEARCH.md Pitfall 5, the same way the existing file documents its fixture dependency ("assets/mock/... fixtures").
- Naming convention: `test_<domain>_<behavior>` function names, e.g. `test_whats_new_valid_token_returns_pending_items`, `test_whats_new_no_token_rejected`, `test_whats_new_wrong_token_rejected` (already specified in RESEARCH.md's Test Map — keep those exact names).
- Split unit vs. integration into two files by filterable name (`test_sync_unit.c` vs. `test_sync_integration.c`) — this is a NEW convention relative to Phase 1 (which had only one `test_storage.c` file covering everything); Phase 1's single-file approach is not followed here specifically because RESEARCH.md Pitfall 5 requires separating network-dependent from pure-logic tests. Note this divergence explicitly in the plan.

---

## Shared Patterns

### HAL/board contract abstraction
**Source:** `src/board/board.h` + `src/board/native_sdl/board.c` + `platformio.ini`'s `build_src_filter`
**Apply to:** `src/sync/http/http_client.h` and its `native_curl`/`esp32` implementations
**Pattern:** one tiny contract header, one implementation per PlatformIO environment, non-selected implementations excluded via `build_src_filter`, never via `#ifdef` soup inside a shared file.

### No-dynamic-allocation, fixed-size-buffer contracts
**Source:** `src/storage/content_api.h` (`char id[16]`, `char title[64]` struct fields; `list_*(out, max_count)` signature)
**Apply to:** `src/sync/sync_client.h`'s content-item struct and the `http_client.h` response struct (`body[4096]` already follows this in RESEARCH.md's draft) — consistent, embedded-friendly memory model across the whole codebase.

### File-level Portuguese doc comments citing decision IDs
**Source:** every existing `src/**/*.c`/`.h` file's top comment block (`board.h`, `native_sdl/board.c`, `content_api.h`, `storage.c`)
**Apply to:** all new `src/sync/**` files — one short paragraph, in Portuguese, naming the driving `D-xx` decision(s) and stating the file's single responsibility. Note: `supabase/**` TypeScript/SQL files have no existing repo convention to inherit — planner should decide whether to keep comments in Portuguese (matching the rest of the codebase) or English (matching Supabase's own ecosystem conventions/RESEARCH.md's cited docs, which are English) — flag as a small open styling question, not a blocker.

### Unity test scaffold
**Source:** `test/test_storage/test_storage.c`
**Apply to:** `test/test_sync/test_sync_unit.c`, `test/test_sync/test_sync_integration.c` — same `#include <unity.h>`, `setUp`/`tearDown` no-ops, `test_*`/`RUN_TEST`/`UNITY_BEGIN`/`UNITY_END` structure.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `supabase/config.toml` | config | — | First Supabase project config file in this repo; no local analog. Use RESEARCH.md Pattern 2's example (`verify_jwt = false` per function) directly. |
| `supabase/functions/register-device/index.ts` | controller (Edge Function) | request-response | First TypeScript/Deno file in this repo — no backend/server code exists anywhere yet (repo is 100% embedded C/LVGL). Use RESEARCH.md's Pattern 2 (`whats-new/index.ts`) as the closest available template, adapted for admin-secret-gated registration instead of bearer-token validation. |
| `supabase/functions/whats-new/index.ts` | controller (Edge Function) | request-response | Same as above — use RESEARCH.md Pattern 2's full code example verbatim as the starting point (it is written for this exact function). |
| `supabase/migrations/<timestamp>_phase2_schema.sql` | migration | CRUD (schema) | First SQL file in this repo. Use RESEARCH.md Pattern 3's example (`devices`/`content_items`/`ota_releases` with `enable row level security` and no policies) verbatim as the starting point. |
| `src/sync/sync_client.c`'s JSON-parsing logic specifically | transform | transform | No JSON-consuming C code exists anywhere in the current codebase to pattern-match against (all existing storage domains read fixed-format `.txt`/directory-listing fixtures, not JSON) — must be written fresh following cJSON's own documented usage pattern, referenced only in RESEARCH.md, not in this repo. |

## Metadata

**Analog search scope:** `src/`, `test/`, `platformio.ini` (full repo excluding `.planning/`, `assets/`)
**Files scanned:** `src/board/board.h`, `src/board/native_sdl/board.c`, `src/board/waveshare_s3_35/board.c` (existence/exclusion only, not read), `src/storage/content_api.h`, `src/storage/storage.c`, `test/test_storage/test_storage.c`, `platformio.ini`
**Pattern extraction date:** 2026-08-27
