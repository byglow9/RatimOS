---
phase: 2
slug: sync-protocol-security-model-cloud-backend
status: validated
nyquist_compliant: true
wave_0_complete: true
created: 2026-08-27
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Unity (PlatformIO built-in), already established in Phase 1 |
| **Config file** | `platformio.ini` — `[env:native_sim]`, `test_build_src = yes` (already present) |
| **Quick run command** | `pio test -e native_sim -f test_sync_unit` (pure-logic, offline — cJSON parsing, no network) |
| **Full suite command** | `pio test -e native_sim -f test_sync_integration` (real-network tests against the deployed Supabase project) |
| **Estimated runtime** | ~5-15s unit; ~10-30s integration (network round-trips) |

**Corrected during planning (#plan-checker):** RESEARCH.md originally proposed a single `test/test_sync/` directory holding both unit and integration test files. The planner corrected this to **two separate directories** — `test/test_sync_unit/` and `test/test_sync_integration/` — because PlatformIO's native Unity runner links every `.c` file inside one `test/<name>/` directory into a single binary with one `main()`; two same-directory files each defining `main()` would collide at link time. All 4 PLAN.md files consistently use the split paths.

---

## Sampling Rate

- **After every task commit:** Run `pio test -e native_sim -f test_sync_unit` (fast, offline)
- **After every plan wave:** Run `pio test -e native_sim -f test_sync_integration` (network-dependent, full auth/transport matrix)
- **Before `/gsd-verify-work`:** Both suites green, plus the manual `grep -rn '"http://' src/sync/` plaintext-HTTP check (expect zero matches) and the live RLS-default-deny curl proof (Plan 04 Task 3)
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Plan | Task | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | Status |
|------|------|------|-------------|------------|-----------------|-----------|---------------------|--------|
| 02-01 | Task 2 (tracer) | 1 | SEC-01 | T-2-01/T-2-05 | Missing admin secret → 401; valid secret → 201 + token issued | integration | `pio test -e native_sim -f test_sync_integration` (`test_register_device_missing_secret_rejected`, `test_register_device_valid_secret_creates_token`) | ⬜ pending |
| 02-02 | Task 2-3 | 2 | SEC-01 | T-2-01/T-2-02/T-2-03 | `whats-new` scopes to caller's device only; missing/foreign token → 401 | integration | `pio test -e native_sim -f test_sync_integration` (extended in Plan 04) + live curl checks | ⬜ pending |
| 02-03 | Task 2 (tdd) | 2 | SEC-01 | — | `ratimos_sync_parse_items` correctly parses/rejects `whats-new` JSON payloads offline | unit | `pio test -e native_sim -f test_sync_unit` (4 canned-JSON tests) | ⬜ pending |
| 02-04 | Task 1 | 3 | SEC-01 | T-2-01/T-2-02/T-2-03 | Full auth-reject matrix: valid token → pending items; missing token → 401; never-registered ("another device's") token → 401 | integration | `pio test -e native_sim -f test_sync_integration` (5 total tests: 2 from Plan 1 + 3 new) | ⬜ pending |
| 02-04 | Task 2 | 3 | SEC-02 | T-2-06 | HTTPS transport succeeds with real cert verification; zero plaintext `http://` literals anywhere in `src/sync/` | integration + static | `pio test -e native_sim -f test_sync_integration && grep -rn '"http://' src/sync/ \| wc -l` (expect exit 0 + count 0) | ⬜ pending |
| 02-04 | Task 3 | 3 | SEC-01 | T-2-05 | RLS default-deny actually blocks a direct PostgREST call using only the publishable key | manual (live curl, one-time architectural proof) | `curl` against `devices`/`content_items` REST endpoints with only `apikey: <publishable_key>` → expect empty/forbidden for every row | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `test/test_sync_integration/test_sync_integration.c` — created in Plan 01 (register-device tests), extended in Plan 04 (whats-new + HTTPS transport tests) — covers SEC-01/SEC-02 end-to-end against the real deployed project
- [x] `test/test_sync_unit/test_sync_unit.c` — created in Plan 03 — pure-logic cJSON parsing tests, no network
- [x] Manual bootstrap precondition documented in Plan 01/04: a test device must be registered (via the register-device tracer itself) and its token available via `getenv("TEST_DEVICE_TOKEN")`/`ADMIN_REGISTRATION_SECRET` before the integration suite can pass
- [x] `supabase/config.toml` with `verify_jwt = false` for both Edge Functions — created in Plan 01 (register-device) and Plan 02 (whats-new)

*All Wave 0 gaps are covered by Plans 01-04 — no additional scaffolding plan needed.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| RLS default-deny actually blocks a direct PostgREST call using only the publishable key | SEC-01/D-09 | One-time architectural proof of the defense-in-depth backstop (Plan 04's own `<threat_model>` explicitly warns: a green `pio test` run alone does NOT prove this — it only proves the primary Edge-Function path works) — confirmed live in Plan 04 Task 3, not scripted into CI for a single-developer project | `curl` the Supabase REST endpoint for `devices`/`content_items` with only the `apikey: <publishable_key>` header, no service-role — expect an empty/forbidden result for every row |

*The admin-secret-rejection check originally scoped here as manual-only was upgraded to a fully automated Unity test (`test_register_device_missing_secret_rejected`, Plan 01) during planning — see the Per-Task Verification Map above.*

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-08-27 (post plan-checker verification pass — no blockers, this file corrected for the test-directory-split documentation drift the checker flagged)
