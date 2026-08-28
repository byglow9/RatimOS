---
phase: 02-sync-protocol-security-model-cloud-backend
plan: 03
subsystem: sync-client
tags: [cjson, libcurl, unity, c, tdd]

# Dependency graph
requires:
  - phase: 02-sync-protocol-security-model-cloud-backend
    provides: "src/sync/http/http_client.h transport contract + native_curl (real) implementation (Plan 1); content_items/whats-new response shape {\"items\":[{\"id\",\"title\",\"type\",\"content_date\",\"url\"}]} (Plan 2)"
provides:
  - "src/sync/sync_client.h: fixed-size ratimos_content_item_t struct + ratimos_sync_whats_new public contract, cJSON-free header"
  - "src/sync/sync_client.c: cJSON-backed ratimos_sync_parse_items (extern-testable, not public) + ratimos_sync_whats_new implementation over http_client.h"
  - "test/test_sync_unit/: offline Unity suite (4 tests) proving JSON parsing safety with zero network dependency"
  - "cJSON v1.7.19 pinned in platformio.ini's native_sim lib_deps -- reusable by any future src/sync/ JSON consumer"
affects: [02-04, phase-7-cloud-content-sync]

# Actuals (#2632)
actuals:
  tokens: 6300
  tasks: 2
  commits: 3

tech-stack:
  added: ["cJSON v1.7.19 (DaveGamble/cJSON, pinned git tag, native_sim only)"]
  patterns:
    - "cJSON confined to sync_client.c only -- sync_client.h stays free of the parsing library's types, mirroring content_api.h's dependency-free header style"
    - "extern-testable-but-not-public helper function (ratimos_sync_parse_items) tested directly by an offline unit suite via a matching extern prototype, exactly like test_storage.c's ratimos_storage_tracks_read_title_or_default convention"
    - "TDD RED/GREEN cycle for a tdd=\"true\" task: failing link (undefined reference) committed first, then the implementation that makes it pass"

key-files:
  created:
    - src/sync/sync_client.h
    - src/sync/sync_client.c
    - test/test_sync_unit/test_sync_unit.c
  modified:
    - platformio.ini

key-decisions:
  - "Reworded sync_client.h's doc comment to avoid the literal string 'cJSON' after the initial draft accidentally referenced it by name in prose, which would have failed the plan's own acceptance check (grep -c cjson\\|cJSON == 0) despite never #including the library -- Rule 1 auto-fix, caught during self-verification before commit."

requirements-completed: [SEC-01, SEC-02]

coverage:
  - id: D1
    description: "ratimos_sync_parse_items returns 0 items, never a crash or out-of-bounds read, for a malformed/non-JSON response body"
    requirement: SEC-01
    verification:
      - kind: unit
        ref: "test/test_sync_unit/test_sync_unit.c#test_sync_parse_items_malformed_json_returns_zero"
        status: pass
    human_judgment: false
  - id: D2
    description: "ratimos_sync_whats_new never attempts to parse the response body when out_status != 200"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "src/sync/sync_client.c ratimos_sync_whats_new: early return before ratimos_sync_parse_items call when response.status_code != 200"
        status: pass
    human_judgment: false
  - id: D3
    description: "cJSON usage confined to sync_client.c -- sync_client.h never includes/mentions cJSON"
    verification:
      - kind: other
        ref: "grep -c 'cjson\\|cJSON' src/sync/sync_client.h (=0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "ratimos_sync_parse_items respects max_count exactly, never copying more than requested even when the response has more items"
    verification:
      - kind: unit
        ref: "test/test_sync_unit/test_sync_unit.c#test_sync_parse_items_respects_max_count"
        status: pass
    human_judgment: false
  - id: D5
    description: "sync_client.c never leaks the parsed cJSON tree -- cJSON_Delete runs on every return path"
    verification:
      - kind: other
        ref: "grep -c cJSON_Delete src/sync/sync_client.c (=3) >= return-statement count inside ratimos_sync_parse_items (=3)"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-08-28
status: complete
---

# Phase 2 Plan 3: Sync Client & Offline JSON Parsing Suite Summary

**cJSON-backed `sync_client.h`/`.c` reading a device's pending whats-new content over the already-proven HTTPS transport, verified by a 4-test offline Unity suite with zero network dependency.**

## Performance

- **Duration:** ~9 min
- **Started:** 2026-08-28T10:14:39-03:00 (immediately following Plan 2's completion)
- **Completed:** 2026-08-28T10:22:45-03:00
- **Tasks:** 2/2
- **Files modified:** 4 (1 modified, 3 created)

## Accomplishments

- `src/sync/sync_client.h` declares a fixed-size, no-dynamic-allocation `ratimos_content_item_t` struct (`id[40]`, `title[64]`, `type[16]`, `content_date[16]`, `url[128]`) and the public `ratimos_sync_whats_new(base_url, device_token, out, max_count, out_status)` contract -- mirrors `content_api.h`'s "contrato unico, sincrono e sem alocacao dinamica" style exactly, and never references the JSON parsing library by type or name.
- `src/sync/sync_client.c` implements `ratimos_sync_parse_items` (extern-linkable, not part of the public header, mirroring `test_storage.c`'s `ratimos_storage_tracks_read_title_or_default` convention): `cJSON_Parse` is NULL-checked before any field access, every string field is copied via `strncpy` with explicit null-termination, and `cJSON_Delete` runs on every return path (malformed JSON, non-array `items`, and the normal success path) -- zero leaks, zero crashes on adversarial input.
- `ratimos_sync_whats_new` builds the full `/functions/v1/whats-new` URL from the caller-supplied `base_url`, calls `ratimos_sync_http_get` (Plan 1's transport), and returns 0 items without ever calling the parser when `status_code != 200` -- a 401 is cleanly distinguishable from a 200 with genuinely 0 pending items via `out_status`.
- `test/test_sync_unit/test_sync_unit.c` -- 4 Unity tests, all offline, no `http_client.h` usage at all: all-fields round-trip, empty array, malformed/non-JSON garbage, and `max_count` capping with a 3-item array.
- `platformio.ini`'s `[env:native_sim]` `lib_deps` now pins `cJSON` to `https://github.com/DaveGamble/cJSON.git#v1.7.19`, matching the existing `lvgl/lvgl@^9.2.2` exact-tag pinning convention.

## Task Commits

1. **Task 1: sync_client.h public contract (interface-first)** - `a4b0db7` (feat)
2. **Task 2 RED: failing offline unit suite (ratimos_sync_parse_items undefined)** - `3f5ebd2` (test)
3. **Task 2 GREEN: sync_client.c implementation, all 4 tests pass** - `b161fd9` (feat)

**Plan metadata:** commit pending (this SUMMARY + STATE.md/ROADMAP.md update)

## TDD Gate Compliance

- RED gate: `3f5ebd2` (`test(02-03): add failing offline unit suite...`) -- confirmed failing via `pio test -e native_sim -f test_sync_unit`: linker error `undefined reference to 'ratimos_sync_parse_items'` (the function genuinely did not exist yet, not a passing-test false negative).
- GREEN gate: `b161fd9` (`feat(02-03): implement sync_client.c...`) -- confirmed passing: all 4 tests `[PASSED]`.
- No REFACTOR commit needed -- the GREEN implementation was already clean on first pass.

## Files Created/Modified

- `src/sync/sync_client.h` - public device-runtime contract: `ratimos_content_item_t`, `ratimos_sync_whats_new`
- `src/sync/sync_client.c` - cJSON-backed `ratimos_sync_parse_items` + `ratimos_sync_whats_new`, calls `http_client.h`'s GET contract
- `test/test_sync_unit/test_sync_unit.c` - offline Unity suite (4 tests), no network
- `platformio.ini` - added cJSON `lib_deps` pinned to `v1.7.19`

## Decisions Made

- Kept the register-device flow entirely out of `sync_client.h`'s contract (per the plan's own assumption) -- this header is reserved for the ongoing device-runtime whats-new contract only, matching RESEARCH.md's Recommended Project Structure.
- Split JSON-parsing logic (`ratimos_sync_parse_items`) into an extern-testable-but-not-public helper rather than exposing it in the header, so the offline unit suite can exercise the parser directly without needing a live HTTP round trip -- same pattern Phase 1 already established for `ratimos_storage_tracks_read_title_or_default`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] sync_client.h's doc comment initially mentioned "cJSON.h" by name, failing its own acceptance check**
- **Found during:** Task 1 self-verification, after Task 2 was underway -- running `grep -c "cjson\|cJSON" src/sync/sync_client.h` (the plan's own acceptance criterion for cJSON confinement) returned 1, not 0, because the header's explanatory prose named the library directly even though it never `#include`d it.
- **Issue:** The literal grep-based acceptance check doesn't distinguish "mentions the library in a comment" from "actually depends on the library" -- but the check is explicit in the plan, so it must pass literally.
- **Fix:** Reworded the comment to say "a biblioteca de parsing JSON" (the JSON parsing library) instead of naming cJSON directly. No functional change -- `sync_client.h` never included cJSON.h at any point.
- **Files modified:** `src/sync/sync_client.h`
- **Verification:** Re-ran `grep -c "cjson\|cJSON" src/sync/sync_client.h` -> 0. Re-ran `gcc -x c -fsyntax-only -std=gnu11 -Isrc src/sync/sync_client.h` -> exits 0. Re-ran `pio test -e native_sim -f test_sync_unit` -> all 4 tests still pass.
- **Committed in:** `b161fd9` (folded into Task 2's GREEN commit, since it was caught during that task's verification pass, before Task 1's commit needed amending)

---

**Total deviations:** 1 auto-fixed (1 bug/self-check)
**Impact on plan:** Purely cosmetic (a doc comment wording fix) with zero functional or test-coverage impact -- caught by the plan's own strict acceptance-criteria grep before it could reach review.

## Issues Encountered

- `src/storage/letters.c:51`'s pre-existing `-Wformat-truncation=` warning (documented in Plan 1's SUMMARY.md) resurfaced again during this plan's builds, since `pio test`/`pio run` rebuild the whole `native_sim` source tree. Still out of scope per SCOPE BOUNDARY -- not fixed, already logged in `deferred-items.md` from Plan 1.
- Installing the pinned `cJSON` git dependency also pulled in the upstream repo's own `fuzzing/` and `test.o` files (part of its repo structure, not a separate PlatformIO package split) -- these compile harmlessly as part of the library but are not used by anything in this project; no action needed, they don't affect the `native_sim` link.

## User Setup Required

None. This plan has zero network dependency in its own test suite (whats-new response shape was already fixed by Plan 2's authoring); `libcurl` dev headers and `cJSON` were both resolved automatically by PlatformIO's Library Manager during the first `pio test`/`pio run` invocation.

## Next Phase Readiness

- `sync_client.h`/`.c` is a stable, tested public contract ready for Plan 4's live integration test (a real HTTPS round trip against the deployed `whats-new` Edge Function using the test device from Plan 2) to call unchanged.
- Phase 7 (Cloud Content Sync) can call `ratimos_sync_whats_new` unchanged once the ESP32 HTTP transport (Plan 1's stub) is filled in for real -- this plan's contract composes with `http_client.h` exactly as designed, no rework anticipated.
- `test/test_sync_unit/` and `test/test_sync_integration/` are now both established, separate PlatformIO test directories (per Pitfall 5) -- Plan 4 should add its live-network tests to `test_sync_integration/`, not create a third directory.

## Self-Check: PASSED

All 3 claimed created files verified present on disk (`src/sync/sync_client.h`, `src/sync/sync_client.c`, `test/test_sync_unit/test_sync_unit.c`); `platformio.ini` modification verified present; all 3 task commits (`a4b0db7`, `3f5ebd2`, `b161fd9`) verified present in git history.

---
*Phase: 02-sync-protocol-security-model-cloud-backend*
*Completed: 2026-08-28*
