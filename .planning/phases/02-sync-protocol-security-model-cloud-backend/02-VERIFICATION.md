---
phase: 02-sync-protocol-security-model-cloud-backend
verified: 2026-08-28T20:00:00Z
status: passed
score: 18/18 must-haves verified
behavior_unverified: 0
overrides_applied: 0
---

# Phase 2: Sync Protocol, Security Model & Cloud Backend Verification Report

**Phase Goal:** The cloud backend and device-authentication protocol exist and are provably secure and testable from a PC build, independent of ESP32 hardware.
**Verified:** 2026-08-28T20:00:00Z
**Status:** passed
**Re-verification:** No — initial verification

## Verification Method

This report does **not** rely on SUMMARY.md prose. Every claim below was independently re-executed against the live Supabase project (`bhqscupdrgfuwitbtlui`) and the local codebase in this session:

- `supabase migration list` and `supabase functions list` re-run live.
- `pio test -e native_sim -f test_sync_integration` re-run live (network, real project) — 6/6 PASS.
- `pio test -e native_sim -f test_sync_unit` re-run offline — 4/4 PASS.
- Live `curl` against `/rest/v1/devices`, `/rest/v1/content_items`, `/rest/v1/ota_releases` using only the publishable key, run fresh in this session.
- `supabase db query --linked` used to independently confirm `ota_releases` (0 rows) and `content_items` (3 rows) live row counts.
- Every source file listed in `files_modified`/`key-files` across all 4 plans was read in full and checked for stub/placeholder patterns, TLS-disabling code, plaintext-scheme URLs, and logging of secrets.

## Goal Achievement

### Observable Truths (ROADMAP Success Criteria — the phase contract)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Supabase schema (devices, content_items, ota_releases) exists with RLS enabled, scoping each device to only its own data | ✓ VERIFIED | `supabase migration list` (this session) shows all 3 migrations (`20260828124835`, `20260828125200`, `20260828130724`) applied to remote. Each table's migration SQL contains an explicit `alter table ... enable row level security;` with zero anon/authenticated policies (`grep -c "create policy"` = 0). Live curl (this session, publishable key only) returns `401 {"code":"42501", "message":"permission denied for table ..."}` on all 3 tables — RLS/grants default-deny confirmed live, not just in migration SQL. |
| 2 | PC-native test client authenticates with a per-device bearer token and gets rejected (401) with no token or another device's token | ✓ VERIFIED | Re-ran `pio test -e native_sim -f test_sync_integration` this session (network, live project): `test_whats_new_no_token_rejected` and `test_whats_new_wrong_token_rejected` both PASS. Single-device project (D-12) tests "another device's token" via a syntactically-valid-but-never-registered token literal, exercising the identical `token_hash` lookup-miss → 401 code path (documented assumption_delta in 02-04-PLAN.md, accepted). |
| 3 | Every backend request/response in the protocol client goes over HTTPS; no endpoint reachable over plaintext HTTP | ✓ VERIFIED | `grep -rn '"http://' src/sync/` (this session) = 0 matches. `CURLOPT_SSL_VERIFYPEER`/`CURLOPT_SSL_VERIFYHOST` present in `native_curl/http_client.c` and never set to a disabling value. Re-ran `test_https_transport_succeeds` this session — real libcurl cert verification against Supabase's live TLS cert succeeds (transport rc=0, non-zero real HTTP status). |
| 4 | A "what's new" endpoint returns pending letters/photos/music for a given device token, verified against the real Supabase project from a PC-native build | ✓ VERIFIED | Re-ran `test_whats_new_valid_token_returns_pending_items` this session — 200 status, exactly 3 items returned, matching one of the 3 seeded titles. Independently confirmed via `supabase db query --linked "select count(*) from content_items"` = 3 (live), `... ota_releases` = 0 (live, matches D-13's schema-only scope). |

**Score (ROADMAP contract):** 4/4 verified.

### Additional Plan-Level Truths (must_haves.truths from PLAN frontmatter — added rigor beyond the roadmap contract)

| # | Truth | Plan | Status | Evidence |
|---|-------|------|--------|----------|
| 5 | `register-device` returns 201+token with valid `X-Admin-Secret`, 401 without it, before any DB insert | 02-01 | ✓ VERIFIED | Re-ran `test_register_device_missing_secret_rejected` / `test_register_device_valid_secret_creates_token` this session — both PASS live. Code in `index.ts` places the secret check as the first statement in the handler. |
| 6 | `devices.token_hash` carries a UNIQUE constraint | 02-01 | ✓ VERIFIED | `supabase/migrations/20260828124835_phase2_devices_schema.sql:12` — `token_hash text not null unique`. |
| 7 | `native_curl/http_client.c` is the only file including `<curl/curl.h>` | 02-01 | ✓ VERIFIED | `grep -rl "curl/curl.h" src` (this session) returns exactly one file: `src/sync/http/native_curl/http_client.c`. |
| 8 | Both HTTP functions reject NULL/empty `url` before calling into libcurl | 02-01 | ✓ VERIFIED | Read `native_curl/http_client.c` — `url_is_empty()` checked immediately after `memset`, before `curl_easy_init()`, in both `ratimos_sync_http_get` and `ratimos_sync_http_post_json`. |
| 9 | `esp32/http_client.c` compiles standalone with zero ESP-IDF/Arduino includes, excluded from native_sim link | 02-01 | ✓ VERIFIED | Re-ran `gcc -fsyntax-only -std=gnu11 -Isrc src/sync/http/esp32/http_client.c` this session — exits 0. `grep -c "esp-idf\|Arduino.h"` = 0. `platformio.ini`'s `build_src_filter` explicitly excludes `-<sync/http/esp32/*>`. |
| 10 | A registered device's plaintext token is never logged or written anywhere except the single register-device response body (backstop truth) | 02-01 | ✓ VERIFIED (static evidence) | `grep -n "console\." supabase/functions/register-device/index.ts` (this session) = 0 matches — the function contains zero logging calls of any kind; `token` is only used to compute `tokenHash` and in the final `JSON.stringify` response. |
| 11 | `content_items`/`ota_releases` carry explicit RLS-enable with zero policies | 02-02 | ✓ VERIFIED | `supabase/migrations/20260828130724_...sql` — both tables followed immediately by `alter table ... enable row level security;`, no `create policy` lines. |
| 12 | `whats-new` never writes `delivered_at` | 02-02 | ✓ VERIFIED | Read `whats-new/index.ts` in full — only a `.select(...)` query against `content_items`, zero `.update()`/`.insert()` calls; `grep -ic "update" supabase/functions/whats-new/index.ts` = 0. |
| 13 | `ota_releases` has zero rows this phase | 02-02 | ✓ VERIFIED | `supabase db query --linked "select count(*) from ota_releases;"` (this session, live) = 0. |
| 14 | Two concurrent whats-new requests (one valid, one just-revoked) are each independently evaluated against current DB state (backstop truth) | 02-02 | ✓ VERIFIED (static evidence) | Read `whats-new/index.ts` — no module-level/global cache variables; every invocation constructs a fresh `createClient` and performs a live `.eq("token_hash", ...)` lookup with no memoization, so a revoked token's very next request is evaluated against current DB state by construction. |
| 15 | `ratimos_sync_parse_items` returns 0 items, never crashes, on malformed/non-JSON body | 02-03 | ✓ VERIFIED | Re-ran `test_sync_parse_items_malformed_json_returns_zero` this session (offline) — PASS. Code NULL-checks `cJSON_Parse`'s return before any field access. |
| 16 | `ratimos_sync_whats_new` never parses the body when `out_status != 200` | 02-03 | ✓ VERIFIED | Read `sync_client.c:75-83` — early return with `*out_status` set, before `ratimos_sync_parse_items` is ever called, whenever `rc != 0` or `response.status_code != 200`. |
| 17 | cJSON usage confined to `sync_client.c`; `sync_client.h` never includes/mentions it | 02-03 | ✓ VERIFIED | `grep -c "cjson\|cJSON" src/sync/sync_client.h` (this session) = 0. |
| 18 | `ratimos_sync_parse_items` respects `max_count` exactly | 02-03 | ✓ VERIFIED | Re-ran `test_sync_parse_items_respects_max_count` this session (offline) — PASS; code `break`s the `cJSON_ArrayForEach` loop once `count >= max_count`. |

**Score (all must-haves, roadmap + plan-level):** 18/18 verified. **behavior_unverified: 0** — no truth in this phase asserts a state transition, cancellation, or cleanup/ordering invariant that presence checks alone couldn't confirm; every truth was either exercised by a live/offline test re-run in this session or confirmed by direct code inspection with no ambiguity (the two "backstop" truths were resolved with explicit static evidence, not left as unverified).

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `supabase/migrations/20260828124835_phase2_devices_schema.sql` | `devices` table + RLS | ✓ VERIFIED | Exists, substantive, applied to remote (confirmed live) |
| `supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql` | service_role grant fix | ✓ VERIFIED | Exists, applied to remote (confirmed live) |
| `supabase/migrations/20260828130724_phase2_content_items_ota_schema.sql` | `content_items`+`ota_releases` + RLS + grants | ✓ VERIFIED | Exists, applied to remote (confirmed live) |
| `supabase/config.toml` | `verify_jwt=false` for both functions | ✓ VERIFIED | Both `[functions.register-device]` and `[functions.whats-new]` blocks present |
| `supabase/functions/register-device/index.ts` | admin-secret gated registration | ✓ VERIFIED | Deployed, ACTIVE, `verify_jwt=false` confirmed live via `supabase functions list` |
| `supabase/functions/whats-new/index.ts` | bearer-token scoped content read | ✓ VERIFIED | Deployed, ACTIVE, `verify_jwt=false` confirmed live |
| `supabase/seed/content_items_seed.sql` | 3 fixed seed rows, label-subquery device_id | ✓ VERIFIED | `select id from devices where label` appears 3x; live row count = 3 |
| `src/sync/http/http_client.h` | HTTPS transport contract | ✓ VERIFIED | Fixed-size struct, no dynamic allocation, mirrors `board.h` style |
| `src/sync/http/native_curl/http_client.c` | real libcurl implementation | ✓ VERIFIED | Compiles, links, exercised live by 6/6 passing integration tests |
| `src/sync/http/esp32/http_client.c` | compiling stub for Phase 7 | ✓ VERIFIED | Compiles standalone with `gcc -fsyntax-only`, excluded from native_sim link, intentional `TODO (Fase 7)` markers (documented, expected, not a debt marker per TBD/FIXME/XXX gate) |
| `src/sync/sync_client.h` / `.c` | fixed-size content-item contract + cJSON parsing | ✓ VERIFIED | Compiles, 4/4 unit tests pass offline, cJSON confined to .c |
| `test/test_sync_integration/test_sync_integration.c` | 6 live Unity tests | ✓ VERIFIED | 6/6 PASS, re-run live this session |
| `test/test_sync_unit/test_sync_unit.c` | 4 offline Unity tests | ✓ VERIFIED | 4/4 PASS, re-run offline this session |
| `platformio.ini` | `-lcurl`, cJSON lib_dep, esp32 exclusion | ✓ VERIFIED | All three present |

### Key Link Verification

| From | To | Via | Status | Details |
|------|-----|-----|--------|---------|
| `test_sync_integration.c` | `register-device` Edge Function | `ratimos_sync_http_post_json` → live HTTPS POST | ✓ WIRED | Re-run live this session, 201/401 both confirmed |
| `test_sync_integration.c` / `sync_client.c` | `whats-new` Edge Function | `ratimos_sync_http_get` → live HTTPS GET | ✓ WIRED | Re-run live this session, 200/401 both confirmed |
| `register-device` Edge Function | `devices` table | service-role client, `.insert()` | ✓ WIRED | Live test creates a real row (201 + token) |
| `whats-new` Edge Function | `content_items` table | service-role client, `.select().eq("device_id",...).is("delivered_at", null)` | ✓ WIRED | Live test returns the 3 real seeded rows scoped to the resolved device |
| Anonymous PostgREST client (publishable key) | `devices`/`content_items`/`ota_releases` | direct REST call | ✓ CONFIRMED BLOCKED | Live curl this session: `401 permission denied (42501)` on all 3 — RLS/grants default-deny holds |
| `sync_client.c` | `http_client.h` (native_curl) | `ratimos_sync_http_get` call | ✓ WIRED | Confirmed by code read + passing live test that exercises this exact call chain |

### Requirements Coverage

| Requirement | Source Plan(s) | Description | Status | Evidence |
|-------------|-----------------|--------------|--------|----------|
| SEC-01 | 02-01, 02-02, 02-03, 02-04 | Every request between device and backend is authenticated with a unique per-device token | ✓ SATISFIED | Full auth-reject matrix live-tested (401 no-token/wrong-token, 201/200 valid); REQUIREMENTS.md marks SEC-01 Complete for Phase 2 |
| SEC-02 | 02-01, 02-04 | All device-backend communication uses HTTPS/TLS, never plaintext | ✓ SATISFIED | Zero plaintext-scheme literals in `src/sync/`; TLS verification live-confirmed against real cert; REQUIREMENTS.md marks SEC-02 Complete |

No orphaned requirements found — `.planning/REQUIREMENTS.md` maps only SEC-01/SEC-02 to Phase 2, and both appear in plan frontmatter `requirements:` fields.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/sync/http/esp32/http_client.c` | 21, 34 | `TODO (Fase 7): implementar via HTTPClient/NetworkClientSecure` | ℹ️ Info | Intentional, documented compiling stub explicitly scoped to Phase 7 per D-05 and the plan's own action text ("TODO-only bodies"). Not a TBD/FIXME/XXX debt marker, does not block this phase's goal. |

No other TBD/FIXME/XXX/HACK/PLACEHOLDER markers, empty stub returns, or hardcoded-empty-data patterns found in any file modified by this phase.

**Prior code review (02-REVIEW.md, 2026-08-28, status: issues_found, 0 critical / 4 warnings / 3 info) factored in:**
- WR-01 (non-constant-time admin-secret comparison), WR-03 (no rate-limiting on register-device) — both are hardening recommendations, not correctness bugs; WR-03 is explicitly breadcrumbed to Phase 10 (`/gsd-secure-phase`) in the plan's own assumptions as an accepted risk for this single-operator gift project.
- WR-02 (silent 4KB response-body truncation could mask "0 pending items") and WR-04 (whats-new's device isolation relies on one `.eq()` filter with no independent enforcement layer) are real defense-in-depth gaps worth addressing before Phase 7 adds real content volume, but do not cause any of the 4 ROADMAP success criteria to fail today — content volume this phase is 3 rows, well under 4KB, and the `.eq("device_id", ...)` filter is sourced exclusively from the server-resolved token lookup (not a client-supplied parameter), which is what SEC-01 requires today.
- None of the 7 findings are blockers; none contradict any of the truths verified above. No override needed — these are legitimate backlog items for Phase 7/10, not gaps in Phase 2's own goal.

### Human Verification Required

None. All truths (including the two "backstop" truths marked non-automatable in plan frontmatter) were resolved with direct, explicit static evidence in this session — no runtime state-transition or cancellation/cleanup/ordering invariant in this phase's scope required a behavioral test beyond what was already re-run live.

### Gaps Summary

No gaps found. All 4 ROADMAP success criteria and all 14 additional plan-level must-have truths were independently re-verified against the live Supabase project and local codebase in this session (not merely trusted from SUMMARY.md prose): 3 migrations confirmed applied to remote, both Edge Functions confirmed `ACTIVE`/`verify_jwt=false`, 6/6 live integration tests and 4/4 offline unit tests re-run and passing, live RLS/grants default-deny confirmed via fresh curl calls returning `401`/`42501` on all 3 tables, zero plaintext-HTTP literals under `src/sync/`, and zero TLS-disabling code. The prior code review's 4 warnings are legitimate hardening backlog items (2 explicitly deferred to Phase 10 by the plan's own design) but do not block phase-goal achievement.

---

_Verified: 2026-08-28T20:00:00Z_
_Verifier: Claude (gsd-verifier)_
