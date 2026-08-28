---
phase: 02-sync-protocol-security-model-cloud-backend
plan: 04
subsystem: testing
tags: [libcurl, unity, https, rls, postgrest, integration-testing]

# Dependency graph
requires:
  - phase: 02-sync-protocol-security-model-cloud-backend
    provides: "devices/content_items/ota_releases schema+RLS, register-device + whats-new Edge Functions (Plans 1-2), sync_client.h/.c HTTPS whats-new client (Plan 3)"
provides:
  - "test_sync_integration/ suite extended to 6 live tests: register-device auth gate (2, from Plan 1), whats-new auth-reject matrix (3, this plan), HTTPS transport proof (1, this plan)"
  - "Live curl proof that an anonymous publishable-key client cannot read devices, content_items, or ota_releases (401 permission-denied on all 3, not just an empty-array RLS response)"
  - "Confirmed the native_sim build output left behind after this phase's testing is the real RatimOS app binary, not a leftover Unity test binary"
affects: [phase-7-cloud-content-sync]

# Actuals (#2632)
actuals:
  tokens: 995
  tasks: 3
  commits: 2

tech-stack:
  added: []
  patterns:
    - "Never-registered-but-syntactically-valid token literal as the single-device-project substitute for 'another device's token' (assumption_delta, D-12) -- same token_hash-lookup-miss -> 401 code path either interpretation would exercise"
    - "Direct ratimos_sync_http_get call (bypassing sync_client.h's higher-level wrapper) used specifically to prove the transport layer's TLS verification succeeds live, isolating SEC-02's guarantee from SEC-01's JSON-parsing/auth logic"

key-files:
  created: []
  modified:
    - test/test_sync_integration/test_sync_integration.c

key-decisions:
  - "RLS live-proof outcome was a genuine 42501 permission-denied error on all 3 tables (not merely an empty [] array) -- an even stronger confirmation than RESEARCH.md's VALIDATION.md draft anticipated, since D-02's disabled 'Automatically expose new tables' setting means the anon role has zero grant on any of these tables, not just zero visible rows under RLS. Both outcomes were pre-approved as passing per the plan's acceptance criteria; documented here since it's the stronger of the two possible passes."

patterns-established: []

requirements-completed: [SEC-01, SEC-02]

coverage:
  - id: D1
    description: "whats-new returns 200 + the 3 seeded pending items for the real registered TEST_DEVICE_TOKEN, verified live from a PC-native build against the deployed Supabase project"
    requirement: SEC-01
    verification:
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_whats_new_valid_token_returns_pending_items"
        status: pass
    human_judgment: false
  - id: D2
    description: "whats-new returns 401 for both a missing token and a syntactically valid but never-registered token"
    requirement: SEC-01
    verification:
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_whats_new_no_token_rejected"
        status: pass
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_whats_new_wrong_token_rejected"
        status: pass
    human_judgment: false
  - id: D3
    description: "No source file under src/sync/ contains a plaintext-scheme URL literal; libcurl TLS verification is never disabled and actually succeeds live against Supabase's real certificate"
    requirement: SEC-02
    verification:
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_https_transport_succeeds"
        status: pass
      - kind: other
        ref: "grep -rn '\"http://' src/sync/ (=0); grep -c CURLOPT_SSL_VERIFYPEER src/sync/http/native_curl/http_client.c (=2); grep -c 'CURLOPT_SSL_VERIFYPEER, 0' (=0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "An anonymous client holding only the publishable key cannot read any row from devices, content_items, or ota_releases"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "live curl -H 'apikey: sb_publishable_...' against .../rest/v1/{devices,content_items,ota_releases} -> HTTP 401 {\"code\":\"42501\",...\"permission denied for table ...\"} on all 3"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-08-28
status: complete
---

# Phase 2 Plan 4: Full Auth-Reject Matrix, HTTPS-Only Guarantee & Live RLS Proof Summary

**All 4 of Phase 2's ROADMAP success criteria proven live against the real deployed Supabase project: 6/6 `test_sync_integration/` tests pass (register-device gate, full whats-new auth-reject matrix, live HTTPS transport), zero plaintext-scheme URLs anywhere in `src/sync/`, and a live curl proof that the publishable-key anon role gets a hard 401 permission-denied on all 3 tables.**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-08-28T13:26:10Z
- **Completed:** 2026-08-28T13:31:00Z (approx)
- **Tasks:** 3/3
- **Files modified:** 1 (test_sync_integration.c only; Task 3 was verification-only, no files)

## Accomplishments

- `test/test_sync_integration/test_sync_integration.c` extended from 2 to 6 Unity tests, all passing live against the real deployed project (`bhqscupdrgfuwitbtlui`): the 2 pre-existing register-device tests (Plan 1), plus 3 new whats-new tests (valid token -> 200 + 3 seeded items; missing token -> 401; never-registered-but-valid-shaped token -> 401) and 1 new HTTPS transport test that calls `ratimos_sync_http_get` directly against the real whats-new URL and asserts transport success + a non-zero real HTTP status, proving libcurl's certificate verification actually works at run time, not just that the right `CURLOPT_SSL_VERIFYPEER`/`VERIFYHOST` options are set in source.
- Static audit confirms `grep -rn '"http://' src/sync/` returns zero matches, and `CURLOPT_SSL_VERIFYPEER` is present and never set to `0` anywhere in `native_curl/http_client.c` -- the HTTPS-only guarantee (SEC-02, ROADMAP criterion #3) is proven both statically and live.
- Live curl checks (publishable key only, no service-role key, no Authorization bearer) against `.../rest/v1/devices`, `.../rest/v1/content_items`, and `.../rest/v1/ota_releases` all returned `HTTP 401` with Postgres error `42501 permission denied for table <name>` -- a genuine access-denial at the grants layer (D-02's disabled "Automatically expose new tables" setting means `anon` was never granted `SELECT` on any of these 3 tables), which is the stronger of the two acceptance-criteria-approved outcomes (the other being an empty `[]` RLS-filtered response). This directly proves ROADMAP Phase 2 success criterion #1 / T-2-06 live, not just via migration-SQL inspection.
- `pio run -e native_sim` re-run after all test commands finished; confirmed via `nm`/`file` that `.pio/build/native_sim/program` is the real RatimOS app binary (zero Unity symbols, 23 board/LVGL symbols present), not a leftover test binary (RESEARCH.md Pitfall 4).

## Task Commits

1. **Task 1: whats-new auth-reject matrix (SEC-01, ROADMAP criteria #2 & #4)** - `ebd57e6` (feat)
2. **Task 2: HTTPS-only guarantee -- live transport check & static scheme audit (SEC-02, ROADMAP criterion #3)** - `5ac417c` (feat)
3. **Task 3: RLS default-deny live proof & final rebuild (ROADMAP criterion #1, T-2-06)** - verification-only, no files modified, no commit (curl checks + `pio run -e native_sim` re-run)

**Plan metadata:** commit pending (this SUMMARY + STATE.md/ROADMAP.md update)

## Files Created/Modified

- `test/test_sync_integration/test_sync_integration.c` - extended from 2 to 6 Unity tests (whats-new auth-reject matrix + live HTTPS transport proof)

## Decisions Made

- Task 3's RLS live-proof came back as a `42501` permission-denied error on all 3 tables rather than an empty `[]` array -- both outcomes were pre-approved as passing per the plan's own acceptance criteria (the plan explicitly anticipated "EITHER an empty JSON array... OR a schema-not-found style error"), and the `42501` result is the stronger proof since it shows `anon` has zero grant at all, not merely zero visible rows under RLS filtering.

## Deviations from Plan

None - plan executed exactly as written. Both the "another device's token" testing strategy (assumption_delta, resolved via a never-registered literal per Pitfall 3 option b) and the RLS-check's dual-acceptable-outcome design were pre-resolved in the plan's own frontmatter/acceptance criteria before execution began.

## Issues Encountered

None.

## User Setup Required

None. Supabase CLI was already logged in and linked to the real project (confirmed at plan start); `.env` already contained `TEST_DEVICE_TOKEN`/`TEST_DEVICE_ID`/`ADMIN_REGISTRATION_SECRET` from Plans 1-2's bootstrap steps.

## Next Phase Readiness

- Phase 2 is now fully complete: all 4/4 ROADMAP success criteria are proven true from a PC-native build against the real, live Supabase project (schema+RLS exists and is live-verified default-deny, the bearer-token auth-reject matrix works end-to-end, every request is HTTPS-only both statically and live, and whats-new returns real pending content for a real device token).
- `src/sync/http/http_client.h` (D-05's swappable transport abstraction), `src/sync/sync_client.h`/`.c` (D-09/D-10's whats-new client), and the `register-device`/`whats-new` Edge Function pair are all stable, tested contracts ready for Phase 7 (Cloud Content Sync) to consume unchanged, once the `esp32/http_client.c` stub (Plan 1) is filled in for real.
- One real test device (`ratimos-whats-new-test`, from Plan 2) and its 3 seeded `content_items` rows remain live in the database as a harmless fixture -- not a blocker for Phase 7's real seeding/testing.
- **Phase-level verification still owed to the orchestrator:** this SUMMARY covers Plan 4's own task-level `<verify>`/`<acceptance_criteria>` blocks, all of which passed. The phase's overall `<verification>`/`<success_criteria>` blocks (in 02-04-PLAN.md, restating the same 4 checks at phase scope) were satisfied by the same evidence documented above, but the orchestrator's own phase-gate step (e.g., `/gsd-verify-work` or equivalent) has not yet run against this phase as a whole -- flagging explicitly per this plan's execution instructions rather than silently assuming it happens automatically.

## Self-Check: PASSED

`test/test_sync_integration/test_sync_integration.c` verified present on disk with 6 `RUN_TEST` entries; both task commits (`ebd57e6`, `5ac417c`) verified present in git history via `git log --oneline`.

---
*Phase: 02-sync-protocol-security-model-cloud-backend*
*Completed: 2026-08-28*
