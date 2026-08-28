---
phase: 02-sync-protocol-security-model-cloud-backend
plan: 02
subsystem: auth
tags: [supabase, edge-functions, deno, postgres, rls]

# Dependency graph
requires:
  - phase: 02-sync-protocol-security-model-cloud-backend
    provides: "devices table + RLS, register-device Edge Function, SHA-256 token hashing pattern (Plan 1)"
provides:
  - "content_items table live on the real Supabase project, RLS enabled, zero anon/authenticated policies"
  - "ota_releases table live on the real Supabase project, RLS enabled, zero rows (Phase 8 defines the real shape later)"
  - "whats-new Edge Function: device-bearer-token-gated, reads content_items scoped by the resolved device_id AND delivered_at IS NULL, never writes delivered_at"
  - "One stable test device ('ratimos-whats-new-test') registered with its token in gitignored .env (TEST_DEVICE_ID/TEST_DEVICE_TOKEN)"
  - "supabase/seed/content_items_seed.sql: fixed, manually-run seed script (3 fictitious rows, one per content type)"
affects: [02-03, 02-04, phase-7-cloud-content-sync, phase-8-ota-concurrent-integration-stress-test]

# Actuals (#2632)
actuals:
  tokens: 1893
  tasks: 3
  commits: 3

tech-stack:
  added: []
  patterns:
    - "Same admin-secret/bearer-token-hash pattern as register-device, applied to a second Edge Function -- SHA-256 hashing and 401-before-any-DB-access are now a repeated, proven pattern across both device-facing endpoints"
    - "Rule 2 auto-fix proactively applied at authoring time: added explicit service_role grants in the same migration as the new tables, instead of waiting to hit the same 42501 error Plan 1 discovered live"

key-files:
  created:
    - supabase/migrations/20260828130724_phase2_content_items_ota_schema.sql
    - supabase/functions/whats-new/index.ts
    - supabase/seed/content_items_seed.sql
  modified:
    - supabase/config.toml
    - .env (gitignored -- TEST_DEVICE_ID/TEST_DEVICE_TOKEN appended, not committed)

key-decisions:
  - "Proactively added service_role grants for content_items/ota_releases in Task 1's own migration (Rule 2), rather than waiting to rediscover Plan 1's identical 42501 permission-denied failure live."
  - "Removed the deno.json/.npmrc that `supabase functions new` scaffolded for whats-new (unused import-map boilerplate referencing packages the handler never imports) to keep whats-new structurally identical to register-device (single index.ts file)."
  - "Ran the seed script via `supabase db query --linked -f supabase/seed/content_items_seed.sql` instead of the Supabase Dashboard's SQL Editor -- functionally the same one-time, manual, non-automated execution the plan's D-11 constraint requires (explicitly NOT `supabase db reset`/any seed pipeline), just via CLI since no browser is available in this execution context."

patterns-established:
  - "Second Edge Function confirms the admin-secret (register-device) vs. bearer-token (whats-new) authentication split is a stable, repeatable pattern for future device-facing endpoints in Phase 7/8."

requirements-completed: [SEC-01]

coverage:
  - id: D1
    description: "content_items and ota_releases tables live on the real Supabase project with RLS enabled and zero anon/authenticated policies"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "grep -c 'enable row level security' supabase/migrations/*_phase2_content_items_ota_schema.sql (=2); grep -c 'create policy' (=0); supabase migration list (remote: 20260828130724 applied)"
        status: pass
    human_judgment: false
  - id: D2
    description: "whats-new Edge Function rejects missing/invalid bearer tokens with 401 before any content_items query, and never writes delivered_at"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "live curl (no Authorization header) -> 401 {\"error\":\"missing bearer token\"}; live curl (Authorization: Bearer not-a-real-token) -> 401 {\"error\":\"invalid token\"}; grep -ic update supabase/functions/whats-new/index.ts (=0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "whats-new returns exactly the seeded pending content_items for the resolved device, scoped by device_id from the hashed token (never a client-supplied parameter)"
    requirement: SEC-01
    verification:
      - kind: other
        ref: "live curl (Authorization: Bearer $TEST_DEVICE_TOKEN) -> 200, {items:[...]} with exactly 3 rows titled 'Carta de teste 1'/'Foto de teste 1'/'Faixa de teste 1'"
        status: pass
    human_judgment: false
  - id: D4
    description: "ota_releases exists with its column structure and zero rows"
    verification:
      - kind: other
        ref: "supabase db query --linked \"select count(*) from ota_releases;\" -> 0"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-08-28
status: complete
---

# Phase 2 Plan 2: Content Schema & Whats-New Endpoint Summary

**content_items + ota_releases tables live with RLS on the real Supabase project, and a bearer-token-scoped whats-new Edge Function proven end-to-end against one real registered test device returning its 3 seeded pending items.**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-08-28T13:03:16Z (immediately following Plan 1's completion)
- **Completed:** 2026-08-28T13:12:41Z
- **Tasks:** 3/3
- **Files modified:** 5 (1 modified, 4 created — 3 committed to git, `.env` gitignored)

## Accomplishments

- `content_items` (id, device_id FK to devices with `on delete cascade`, title, type check-constrained to `letter`/`photo`/`music`, content_date, url, delivered_at nullable, created_at) and `ota_releases` (id, version, url, released_at) both live on the real hosted Supabase project (`bhqscupdrgfuwitbtlui`), RLS enabled with zero anon/authenticated policies — explicit default-deny in the migration file itself, matching Plan 1's `devices` convention exactly.
- `whats-new` Edge Function deployed and battle-tested live: `verify_jwt = false` confirmed immediately after deploy (mirroring Plan 1's register-device verification discipline); missing/malformed `Authorization` header returns 401 before any DB access; a syntactically valid but never-registered bearer token hashes correctly and still returns 401 (`invalid token`) after the `devices` lookup misses; a valid token resolves `device_id` from the hash lookup and returns exactly that device's pending (`delivered_at IS NULL`) `content_items` rows — read-only, never writes `delivered_at` (confirmed by `grep -ic update` returning 0).
- One stable test device (`label='ratimos-whats-new-test'`) registered via Plan 1's `register-device` endpoint; its `device_id`/`token` appended to gitignored `.env` as `TEST_DEVICE_ID`/`TEST_DEVICE_TOKEN`, never printed to terminal, log, or committed file.
- `supabase/seed/content_items_seed.sql` — 3 fictitious rows (one per content type, generic placeholder titles mirroring Phase 1's `assets/mock/letters/carta1.txt` convention), each resolving `device_id` via the `label` subquery rather than a hardcoded UUID literal — run once live via `supabase db query --linked -f ...` (a manual, one-time execution equivalent to the Dashboard SQL Editor, deliberately not part of any automated CLI seed/reset pipeline).
- End-to-end live proof: `curl -H "Authorization: Bearer $TEST_DEVICE_TOKEN" .../functions/v1/whats-new` returns HTTP 200 with exactly 3 items titled "Carta de teste 1", "Foto de teste 1", "Faixa de teste 1".

## Task Commits

1. **Task 1: content_items & ota_releases schema, RLS enabled (D-10/D-12/D-13)** - `e4852d5` (feat)
2. **Task 2: whats-new Edge Function — bearer-token-scoped pending content (D-09/D-10)** - `df85e28` (feat)
3. **Task 3: One-time test-device bootstrap & fixed content_items seed (D-06/D-11)** - `041d2e1` (feat)

**Plan metadata:** commit pending (this SUMMARY + STATE.md/ROADMAP.md update)

## Files Created/Modified

- `supabase/migrations/20260828130724_phase2_content_items_ota_schema.sql` - `content_items` + `ota_releases` tables, RLS enabled, service_role grants
- `supabase/config.toml` - added `[functions.whats-new]` with `verify_jwt = false`
- `supabase/functions/whats-new/index.ts` - bearer-token-validated, device-scoped pending-content endpoint
- `supabase/seed/content_items_seed.sql` - fixed, manually-run seed script (D-11)
- `.env` (gitignored, not committed) - appended `TEST_DEVICE_ID`/`TEST_DEVICE_TOKEN`

## Decisions Made

- Proactively applied Plan 1's exact `service_role` grant fix to `content_items`/`ota_releases` within Task 1's own migration, rather than waiting to rediscover the identical 42501 permission-denied failure live during Task 2's testing — this project's disabled "Automatically expose new tables" setting (D-02) suppresses default grants on every new table, not just `devices`.
- Removed `supabase functions new`'s scaffolded `deno.json`/`.npmrc` for `whats-new` (declared `@supabase/functions-js`/`@supabase/server` imports the handler never uses) so `whats-new`'s file structure matches `register-device`'s single-`index.ts` convention exactly.
- Ran the seed script via `supabase db query --linked -f ...` rather than literally pasting into the Supabase Dashboard's browser SQL Editor (no browser available in this execution context) — this is the same one-time, manual, non-automated execution D-11 requires; the constraint being guarded against is `supabase db reset`/any seed pipeline that would auto-run on every reset, not the specific input method.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical Functionality] Added service_role grants for content_items/ota_releases proactively**
- **Found during:** Task 1 (writing the migration), based on Plan 1's SUMMARY documenting the identical issue on `devices`
- **Issue:** This project's "Automatically expose new tables" setting (D-02) being disabled also suppresses Postgres's default table-level `GRANT` to `service_role` on any newly created table — not just `devices`. Without an explicit grant, `whats-new`'s service-role client would hit the same 42501 permission-denied error Plan 1 discovered live, before ever reaching the RLS check.
- **Fix:** Added `grant select, insert, update, delete on content_items to service_role;` and the same for `ota_releases` in the same migration file as the table creation (Task 1), rather than a separate follow-up migration.
- **Files modified:** `supabase/migrations/20260828130724_phase2_content_items_ota_schema.sql`
- **Verification:** `supabase db push` succeeded; Task 2's live curl smoke tests against `whats-new` (which uses this exact grant) succeeded on the first deploy with no permission errors.
- **Committed in:** `e4852d5` (Task 1 commit)

**2. [Rule 1 - Bug/Cleanup] Removed stale scaffold files from `supabase functions new`**
- **Found during:** Task 2, after `supabase functions new whats-new` auto-generated a `deno.json` and `.npmrc` alongside `index.ts`
- **Issue:** The generated `deno.json` declared import-map entries (`@supabase/functions-js`, `@supabase/server`) that the actual handler (written to mirror `register-device`'s `Deno.serve` + `npm:@supabase/supabase-js@2` pattern) never imports — dead, potentially confusing boilerplate that also diverged from `register-device`'s file structure.
- **Fix:** Deleted both files, redeployed; `supabase functions list` confirmed `import_map` flipped from `true` to `false`, matching `register-device`'s shape.
- **Files modified:** `supabase/functions/whats-new/deno.json` (deleted), `supabase/functions/whats-new/.npmrc` (deleted)
- **Verification:** Redeploy succeeded; `verify_jwt=false` reconfirmed; both curl auth-gate smoke tests still passed after redeploy.
- **Committed in:** `df85e28` (Task 2 commit — the deletions predate the commit and were never staged/tracked, so no separate removal diff appears in git history)

---

**Total deviations:** 2 auto-fixed (1 missing-critical, 1 bug/cleanup)
**Impact on plan:** Both auto-fixes were necessary for correctness (Rule 2) or cleanliness/consistency (Rule 1). No scope creep; RLS default-deny architecture (D-09) and the plan's specified files (`supabase/config.toml`, `supabase/functions/whats-new/index.ts`) are unchanged in shape.

## Issues Encountered

None beyond the two auto-fixed deviations documented above.

## User Setup Required

None. Supabase CLI was already logged in and linked (confirmed at plan start); the admin secret needed for Task 3's device registration was already present in `.env` from Plan 1.

## Next Phase Readiness

- `content_items`/`ota_releases` schema and the `whats-new` endpoint are stable and ready for Plan 3's `sync_client.h`/`sync_client.c` (cJSON parsing, HTTP consumption of this exact response shape) to build on unchanged.
- Plan 4's live RLS-verification (a raw PostgREST call using only the publishable key, expected to return zero rows against `content_items`/`ota_releases`) has a real, populated table to test against now.
- One real test device (`ratimos-whats-new-test`) with 3 seeded pending `content_items` rows now exists in the live database — this is the fixture Plan 4's full C-client integration test is expected to assert against (per this plan's `key_links`).
- `ota_releases` remains genuinely empty (0 rows) — no synthetic OTA test data was seeded, matching D-13's explicit "Phase 8 decides the real shape later" scope boundary.

## Self-Check: PASSED

All 5 claimed files verified present on disk (4 tracked in git, `.env` gitignored as intended); all 3 task commits (`e4852d5`, `df85e28`, `041d2e1`) verified present in git history; no secrets found in any commit diff or file content.

---
*Phase: 02-sync-protocol-security-model-cloud-backend*
*Completed: 2026-08-28*
