---
phase: 2
slug: sync-protocol-security-model-cloud-backend
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| **Quick run command** | `pio test -e native_sim -f test_sync_unit` (pure-logic, offline) |
| **Full suite command** | `pio test -e native_sim -f test_sync` (includes real-network integration tests against the deployed Supabase project) |
| **Estimated runtime** | ~5-15s unit; ~10-30s full (network round-trips) |

---

## Sampling Rate

- **After every task commit:** Run `pio test -e native_sim -f test_sync_unit`
- **After every plan wave:** Run `pio test -e native_sim -f test_sync` (full suite, including integration)
- **Before `/gsd-verify-work`:** Full suite green, plus the manual `grep -rn '"http://' src/sync/` plaintext-HTTP check (expect zero matches)
- **Max feedback latency:** ~30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | SEC-01 | T-2-01 | Valid per-device token → 200 with that device's pending content | integration | `pio test -e native_sim -f test_sync -- test_whats_new_valid_token_returns_pending_items` | ❌ W0 | ⬜ pending |
| 02-01-02 | 01 | 1 | SEC-01 | T-2-02 | Missing token → 401 | integration | `pio test -e native_sim -f test_sync -- test_whats_new_no_token_rejected` | ❌ W0 | ⬜ pending |
| 02-01-03 | 01 | 1 | SEC-01 | T-2-03 | Foreign/never-registered token → 401 | integration | `pio test -e native_sim -f test_sync -- test_whats_new_wrong_token_rejected` | ❌ W0 | ⬜ pending |
| 02-01-04 | 01 | 1 | SEC-02 | T-2-06 | Client never constructs a plaintext `http://` URL | unit (static check) | `grep -rn '"http://' src/sync/` (expect zero matches) | ❌ W0 | ⬜ pending |
| 02-01-05 | 01 | 1 | SEC-02 | T-2-06 | HTTPS call to the real endpoint succeeds with cert verification enabled | integration | `pio test -e native_sim -f test_sync -- test_https_transport_succeeds` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `test/test_sync/test_sync_unit.c` — pure-logic tests (JSON parsing via cJSON, no network) — covers structural correctness ahead of SEC-01/SEC-02
- [ ] `test/test_sync/test_sync_integration.c` — real-HTTPS tests against the deployed Supabase project — covers SEC-01/SEC-02 end-to-end
- [ ] A documented manual bootstrap step (register at least one test device via curl, store its token in `.env`) that must exist before the integration suite can pass — a test precondition, not a code gap, but must be written down for future reference
- [ ] `supabase/config.toml` with `verify_jwt = false` for both Edge Functions — a config gap that blocks the integration tests from ever passing until it exists

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Registration endpoint rejects requests missing the admin secret | SEC-01 (defense-in-depth for D-07) | Requires a live curl call against the deployed function with a deliberately wrong/missing `X-Admin-Secret` header — best confirmed once during phase-gate UAT rather than scripted for a single-developer project | `curl -i -X POST <function-url>/register-device` (no header) → expect 401; repeat with a wrong value → expect 401 |
| RLS default-deny actually blocks a direct PostgREST call using only the publishable key | SEC-01/D-09 | One-time architectural proof, not a regression-prone code path — worth confirming live once rather than building permanent test infra for it | `curl` the Supabase REST endpoint for `devices`/`content_items` with only the `apikey: <publishable_key>` header, no service-role — expect an empty/forbidden result for every row |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
