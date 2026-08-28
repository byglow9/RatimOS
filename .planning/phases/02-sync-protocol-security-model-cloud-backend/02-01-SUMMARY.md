---
phase: 02-sync-protocol-security-model-cloud-backend
plan: 01
subsystem: auth
tags: [supabase, edge-functions, deno, postgres, rls, libcurl, platformio, https]

# Dependency graph
requires:
  - phase: 01-shell-storage-api-simulator-first-app-shells
    provides: native_sim PlatformIO environment, board/HAL directory-per-concern convention, Unity test scaffold (test/test_storage/test_storage.c)
provides:
  - "devices table live on the real Supabase project (bhqscupdrgfuwitbtlui), RLS enabled, zero anon/authenticated policies"
  - "register-device Edge Function: admin-secret-gated, SHA-256 token hashing, opaque UUID token minted and returned exactly once"
  - "src/sync/http/http_client.h contract + native_curl (real) and esp32 (compiling stub) implementations -- D-05 swappable HTTPS transport"
  - "platformio.ini native_sim build wired for libcurl (-lcurl build flag, esp32 stub excluded from link)"
  - "test/test_sync_integration/ Unity suite proving the 401/201 auth-gate paths against the live deployed project"
affects: [02-02, 02-03, 02-04, phase-7-cloud-content-sync]

# Actuals (#2632)
actuals:
  tokens: 4153
  tasks: 3
  commits: 2

tech-stack:
  added: [libcurl (native_sim only), Supabase CLI 2.116.0, "@supabase/supabase-js@2 (npm: Deno specifier, inside Edge Functions only)"]
  patterns:
    - "HAL/board-style contract for HTTPS transport (http_client.h + one .c implementation per PlatformIO environment, non-selected implementation excluded via build_src_filter -- mirrors src/board/)"
    - "verify_jwt = false + manual header-based auth inside the Edge Function handler (opaque bearer/admin-secret tokens, never Supabase JWTs)"
    - "RLS enabled with zero anon/authenticated policies as a default-deny backstop; service_role (used only inside Edge Functions) is the sole intended access path"

key-files:
  created:
    - supabase/migrations/20260828124835_phase2_devices_schema.sql
    - supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql
    - supabase/config.toml
    - supabase/functions/register-device/index.ts
    - src/sync/http/http_client.h
    - src/sync/http/native_curl/http_client.c
    - src/sync/http/esp32/http_client.c
    - test/test_sync_integration/test_sync_integration.c
    - .planning/phases/02-sync-protocol-security-model-cloud-backend/deferred-items.md
  modified:
    - platformio.ini
    - .gitignore

key-decisions:
  - "Package-legitimacy checkpoint for @supabase/supabase-js resolved as a confirmed false positive (25M+ weekly downloads, official supabase GitHub org) -- coordinator approved before any Edge Function code was written."
  - "Added a second migration granting service_role explicit table privileges on devices, because this project's 'Automatically expose new tables' setting (D-02, deliberately disabled) also suppresses Postgres's default table-level GRANT -- service_role hit 42501 permission-denied before ever reaching RLS. This is a Rule 3 auto-fix, not an architectural change: anon/authenticated remain ungranted."

patterns-established:
  - "src/sync/http/ as the third sibling directory (after board/, storage/) in this project's per-concern HAL layering -- future sync modules (sync_client.h/.c in later plans) compose on top of this transport contract without touching it."
  - "Two-migration convention when a schema change and a grants/permissions fix are logically separate concerns -- keeps the original schema migration's intent auditable."

requirements-completed: [SEC-01, SEC-02]

coverage:
  - id: D1
    description: "devices table live on the real hosted Supabase project with RLS enabled and zero anon/authenticated policies (default-deny)"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "supabase migration list (remote: 20260828124835, 20260828125200 both applied)"
        status: pass
    human_judgment: false
  - id: D2
    description: "register-device Edge Function enforces the X-Admin-Secret gate: 401 immediately (before any DB access) without it, 201 + fresh opaque token with it"
    requirement: SEC-01
    verification:
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_register_device_missing_secret_rejected"
        status: pass
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_register_device_valid_secret_creates_token"
        status: pass
    human_judgment: false
  - id: D3
    description: "native_curl HTTP client performs a real HTTPS round trip against the live Supabase project with TLS verification left at its secure defaults (never disabled)"
    requirement: SEC-02
    verification:
      - kind: integration
        ref: "test/test_sync_integration/test_sync_integration.c#test_register_device_valid_secret_creates_token"
        status: pass
      - kind: other
        ref: "grep -c curl/curl.h src/sync/http/native_curl/http_client.c (=1); grep -rl curl/curl.h src (no other files)"
        status: pass
    human_judgment: false
  - id: D4
    description: "esp32 HTTP transport stub proves D-05's abstraction is genuinely swappable today -- compiles standalone with zero ESP-IDF/Arduino includes, excluded from the native_sim link"
    verification:
      - kind: other
        ref: "gcc -fsyntax-only -std=gnu11 -Isrc src/sync/http/esp32/http_client.c && pio run -e native_sim"
        status: pass
    human_judgment: false

duration: 28min
completed: 2026-08-28
status: complete
---

# Phase 2 Plan 1: Device Registration Tracer Summary

**Opaque-token device auth end-to-end: Postgres `devices` table + RLS on the real Supabase project, an admin-secret-gated `register-device` Edge Function, and a real libcurl-backed C HTTPS client proven against the live endpoint.**

## Performance

- **Duration:** ~28 min (across two coordinator checkpoints: package-legitimacy approval, tracer feedback gate)
- **Started:** 2026-08-28T12:36:50Z
- **Completed:** 2026-08-28T13:03:16Z
- **Tasks:** 3/3
- **Files modified:** 10 (2 modified, 8 created) + this SUMMARY/STATE/ROADMAP metadata

## Accomplishments

- `devices` table (id, token_hash unique, label, created_at, last_seen_at) live on the real hosted Supabase project (`bhqscupdrgfuwitbtlui`), RLS enabled with zero anon/authenticated policies -- explicit default-deny visible in the migration file itself, not left to the project's invisible auto-RLS trigger alone.
- `register-device` Edge Function deployed and battle-tested live: `verify_jwt = false` confirmed both immediately after deploy and after a subsequent redeploy (guards against the known CLI reset issue); `X-Admin-Secret` check is the first line of the handler (401 before any DB access); SHA-256 hashes the opaque `crypto.randomUUID()` token before persisting; the plaintext token is returned exactly once in the 201 response body.
- `src/sync/http/http_client.h` HAL-style contract (mirroring `src/board/board.h`) with a fixed-size, no-dynamic-allocation `ratimos_http_response_t` struct, implemented for real by `src/sync/http/native_curl/http_client.c` (libcurl, `CURLOPT_SSL_VERIFYPEER`/`VERIFYHOST` always left at secure defaults) and stubbed for Phase 7 by `src/sync/http/esp32/http_client.c` (zero ESP-IDF/Arduino includes, excluded from the `native_sim` link).
- `test/test_sync_integration/test_sync_integration.c` proves both auth-gate paths against the real deployed project: missing secret -> 401, valid secret -> 201 with a token in the body.

## Task Commits

1. **Task 1: Confirm `@supabase/supabase-js` package legitimacy** - checkpoint only, no commit (coordinator approved: 25M+ weekly downloads, official `supabase` GitHub org, false-positive "too-new" heuristic)
2. **Task 2: End-to-end "register a device" -- schema, Edge Function & C HTTPS client** - `6ff362c` (feat)
3. **Task 3: Compiling-but-unlinked esp32 HTTP stub (D-05 swappability proof)** - `e3e2fbe` (feat)

**Plan metadata:** commit pending (this SUMMARY + STATE.md/ROADMAP.md update)

## Files Created/Modified

- `supabase/migrations/20260828124835_phase2_devices_schema.sql` - `devices` table + `enable row level security`, no policies
- `supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql` - Rule 3 auto-fix: explicit `service_role` grants (see Deviations)
- `supabase/config.toml` - `verify_jwt = false` for `register-device`
- `supabase/functions/register-device/index.ts` - admin-secret-gated registration, SHA-256 token hashing, service-role insert
- `src/sync/http/http_client.h` - HTTPS transport contract (D-05)
- `src/sync/http/native_curl/http_client.c` - libcurl-backed implementation, sole file in the repo including the libcurl header
- `src/sync/http/esp32/http_client.c` - compiling-but-unlinked stub for Phase 7
- `test/test_sync_integration/test_sync_integration.c` - Unity integration suite (network required, reads `ADMIN_REGISTRATION_SECRET` from env)
- `platformio.ini` - `-lcurl` build flag; `build_src_filter` exclusion for `sync/http/esp32/*`
- `.gitignore` - added `supabase/.temp/` (local Supabase CLI cache, contains the project's pooler connection string -- not a secret but not meant for git)
- `.planning/phases/02-sync-protocol-security-model-cloud-backend/deferred-items.md` - logged an out-of-scope pre-existing warning (see Issues Encountered)

## Decisions Made

- Confirmed `@supabase/supabase-js`'s SUS package-legitimacy verdict as a false positive per the coordinator's explicit review of npmjs.com and the official GitHub org -- proceeded with the `npm:@supabase/supabase-js@2` Deno specifier as RESEARCH.md recommended.
- Added a second migration (grants) rather than folding the fix into the first migration, keeping the original schema migration's intent (RLS default-deny) auditable and undiluted by an unrelated permissions bug found during execution.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `service_role` had no table-level grant on `devices`, causing every insert to fail with Postgres 42501 before RLS was ever reached**
- **Found during:** Task 2, first live curl smoke test of `register-device` with a valid `X-Admin-Secret` (returned `{"error":"registration failed"}` instead of 201)
- **Issue:** This project's "Automatically expose new tables" setting (D-02) is deliberately disabled, which also suppresses Postgres's default table-level `GRANT` to `service_role` for newly created tables. `service_role` is meant to bypass RLS entirely and be the sole intended access path (D-09), but with zero grant it couldn't even reach the RLS check -- confirmed via the exact Postgres error text `permission denied for table devices` (error code 42501, a grants issue, not an RLS-policy issue).
- **Fix:** Added `supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql` granting `select, insert, update, delete on devices to service_role`. `anon`/`authenticated` remain ungranted -- the default-deny posture (T-2-06) is untouched.
- **Files modified:** `supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql`
- **Verification:** Re-ran the live curl smoke test after `supabase db push` -- 201 with a fresh token; both integration tests subsequently passed.
- **Committed in:** `6ff362c` (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Necessary for the tracer to work at all -- without this grant, `register-device` would never succeed regardless of code correctness. No scope creep; RLS default-deny architecture (D-09) is unchanged.

## Issues Encountered

- `src/storage/letters.c:51` has a pre-existing `-Wformat-truncation=` compiler warning (Phase 1 code, unrelated to this plan's file scope), surfaced incidentally because `pio test` rebuilds the whole `native_sim` source tree. Logged to `deferred-items.md`, not fixed (out of scope per SCOPE BOUNDARY).
- Diagnosing the `service_role` grant issue required a debug round-trip: temporarily added `error.message` and env-key-name introspection (`Object.keys(Deno.env.toObject())`, then truncated key prefixes) to the Edge Function's error response to distinguish an RLS-policy failure from a grants failure and confirm the auto-injected `SUPABASE_SERVICE_ROLE_KEY` was in fact a valid new-format `sb_secret_...` key. All debug output was removed and the function redeployed clean before the final commit -- no secret value (full key, admin secret, or device token) was ever logged, committed, or printed beyond a 12-character prefix used transiently during interactive debugging.

## User Setup Required

None beyond what the coordinator already completed before this plan started (Supabase CLI login + `supabase link --project-ref bhqscupdrgfuwitbtlui`, confirmed at plan start).

## Next Phase Readiness

- `src/sync/http/*` transport contract is stable and ready for Plan 3's `sync_client.h`/`sync_client.c` (cJSON parsing, `whats-new` endpoint consumption) to build on unchanged.
- `devices` schema and the registration flow are the foundation Plan 2 (`content_items`/`ota_releases` schema) and Plan 4 (RLS live-verification from a raw PostgREST call) will extend -- no rework anticipated.
- One real test device now exists in the live `devices` table from this plan's smoke tests (label `ci-smoke-test`/`ci-test`) -- harmless leftover test data, not a blocker for later plans.

## Self-Check: PASSED

All 10 claimed files verified present on disk; both task commits (`6ff362c`, `e3e2fbe`) verified present in git history.

---
*Phase: 02-sync-protocol-security-model-cloud-backend*
*Completed: 2026-08-28*
