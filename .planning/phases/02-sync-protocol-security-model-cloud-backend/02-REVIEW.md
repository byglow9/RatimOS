---
phase: 02-sync-protocol-security-model-cloud-backend
reviewed: 2026-08-28T00:00:00Z
depth: standard
files_reviewed: 15
files_reviewed_list:
  - .gitignore
  - platformio.ini
  - src/sync/http/esp32/http_client.c
  - src/sync/http/http_client.h
  - src/sync/http/native_curl/http_client.c
  - src/sync/sync_client.c
  - src/sync/sync_client.h
  - supabase/config.toml
  - supabase/functions/register-device/index.ts
  - supabase/functions/whats-new/index.ts
  - supabase/migrations/20260828124835_phase2_devices_schema.sql
  - supabase/migrations/20260828125200_phase2_devices_service_role_grant.sql
  - supabase/migrations/20260828130724_phase2_content_items_ota_schema.sql
  - supabase/seed/content_items_seed.sql
  - test/test_sync_integration/test_sync_integration.c
  - test/test_sync_unit/test_sync_unit.c
findings:
  critical: 0
  warning: 4
  info: 3
  total: 7
status: issues_found
---

# Phase 02: Code Review Report

**Reviewed:** 2026-08-28T00:00:00Z
**Depth:** standard
**Files Reviewed:** 15
**Status:** issues_found

## Summary

Reviewed the sync protocol transport layer (http_client abstraction, sync_client), the two Supabase Edge Functions (register-device, whats-new), the RLS/grant migrations, the seed script, and both test suites.

The core security posture checked out on every point called out in the review brief: RLS is enabled with zero anon/authenticated policies on all three new tables (default-deny confirmed), `service_role` is scoped exclusively to Edge Functions and explicitly granted after the "Automatically expose new tables" workaround, no plaintext secrets or tokens are hardcoded or logged anywhere (device tokens are hashed with SHA-256 before persistence, and the plaintext token is returned exactly once from register-device), TLS certificate verification (`CURLOPT_SSL_VERIFYPEER`/`CURLOPT_SSL_VERIFYHOST`) is explicitly enabled and never disabled in `native_curl/http_client.c`, the admin-secret check in `register-device` runs before any DB access, `whats-new` scopes `content_items` exclusively by the `device_id` resolved from the caller's own bearer token (never a client-supplied ID), both Edge Functions use the parameterized supabase-js query builder (no raw SQL, no injection surface), and only `src/sync/http/native_curl/http_client.c` includes `<curl/curl.h>` — the `esp32` stub and the `http_client.h` contract stay libcurl-free as intended.

No blockers were found. A handful of warnings and info items are worth addressing, mostly around defense-in-depth on the admin-secret comparison, silent truncation behavior in the fixed 4KB response buffer, and a couple of minor contract/robustness gaps in the transport stub and public API.

## Warnings

### WR-01: Non-constant-time comparison of the admin registration secret

**File:** `supabase/functions/register-device/index.ts:29`
**Issue:** `providedSecret !== expectedSecret` is a standard JS string comparison, which short-circuits on the first mismatched character. This is a textbook timing side-channel for secret comparison. While the practical exploitability over the public internet against a Deno Edge Function is low (network jitter dwarfs the per-character timing signal), this is the single authentication gate protecting device registration (and therefore the entire token-issuance pipeline), so it's worth hardening rather than relying on network noise as the only mitigation.
**Fix:**
```ts
function timingSafeEqual(a: string, b: string): boolean {
  const enc = new TextEncoder();
  const aBytes = enc.encode(a);
  const bBytes = enc.encode(b);
  if (aBytes.length !== bBytes.length) return false;
  let diff = 0;
  for (let i = 0; i < aBytes.length; i++) diff |= aBytes[i] ^ bBytes[i];
  return diff === 0;
}
// ...
if (!expectedSecret || !providedSecret || !timingSafeEqual(providedSecret, expectedSecret)) {
```

### WR-02: Silent truncation of the HTTP response body can masquerade as "zero pending items"

**File:** `src/sync/http/native_curl/http_client.c:11-23`, `src/sync/sync_client.h:14-18` (`char body[4096]`)
**Issue:** `write_cb` silently truncates any response body larger than 4095 bytes and still tells libcurl the full chunk was consumed (comment on line 22 acknowledges this), so the transport layer never surfaces a truncation signal. If `whats-new`'s JSON payload ever exceeds ~4KB (plausible once real devices accumulate pending `content_items`, since this phase's `whats-new` never marks rows `delivered_at` and Phase 7 hasn't landed yet), `cJSON_Parse` in `ratimos_sync_parse_items` will either fail outright on the truncated JSON (returning 0, indistinguishable from a genuine "0 pending items" `200` response) or successfully parse a partial `items` array, silently dropping the tail. Either way, the caller has no way to detect that content was lost — this is a data-loss-adjacent correctness bug, not just a size limit.
**Fix:** At minimum, have `write_cb` record whether truncation occurred (e.g., an `out->truncated` flag) and have `ratimos_sync_whats_new` propagate it via `out_status` (e.g., a sentinel status) so the caller can distinguish "genuinely empty" from "response didn't fit." Longer-term, size the buffer against a realistic worst case or switch to a growable buffer.

### WR-03: No rate limiting / brute-force protection on the admin-secret endpoint

**File:** `supabase/functions/register-device/index.ts:26-34`
**Issue:** `register-device` has no attempt throttling of its own. An attacker who can reach the function URL can make unlimited guesses against `ADMIN_REGISTRATION_SECRET` with no lockout, backoff, or logging of failed attempts. Since a successful guess grants the ability to mint a valid device bearer token (which in turn unlocks `whats-new`), this endpoint is effectively the root of trust for the whole sync system.
**Fix:** Consider a minimal application-level guard (e.g., track failed-attempt counts per IP in a small table with a short-lived lockout window), or confirm and document that Supabase's platform-level Edge Function rate limits are sufficient for this threat model.

### WR-04: `whats-new`'s device isolation relies entirely on one `.eq("device_id", ...)` clause with no independent enforcement layer

**File:** `supabase/functions/whats-new/index.ts:70-74`
**Issue:** Because `service_role` bypasses RLS entirely (by design, per D-09), the *only* thing preventing one device from seeing another device's `content_items` is the manually-written `.eq("device_id", device.id)` filter on this one query. There is no RLS policy, database constraint, or second layer of defense — a future edit to this function (e.g., adding a second query path, refactoring the filter, or a copy-paste of this pattern into a new endpoint) could silently drop the filter and leak cross-device content with no failing test to catch it structurally (the current tests only exercise the happy path and auth-rejection cases, not cross-device isolation).
**Fix:** Not a bug today, but worth hardening before more endpoints are added on this pattern: add an integration test that registers two devices, seeds content for each, and asserts device A's token never returns device B's items; consider a lightweight assertion helper/lint rule flagging any `content_items`/`devices`-adjacent query missing a `device_id` filter.

## Info

### IN-01: `esp32` transport stub returns success-shaped-but-uninitialized `out` on failure

**File:** `src/sync/http/esp32/http_client.c:16-24, 26-37`
**Issue:** Both stub functions set `out->status_code = 0` but never touch `out->body`/`out->body_len` (unlike `native_curl/http_client.c`, which `memset`s the whole struct up front). Since the stub always returns `-1`, well-behaved callers shouldn't read `out->body`, but this is an easy landmine to inherit if the real Fase 7 implementation is written by copying this stub without noticing the missing zero-init, or if a caller ever inspects `out->body` after checking `rc` incorrectly.
**Fix:**
```c
int ratimos_sync_http_get(const char *url, const char *bearer_token,
                           ratimos_http_response_t *out)
{
    (void) url;
    (void) bearer_token;
    memset(out, 0, sizeof(*out));
    return -1;
}
```

### IN-02: `ratimos_sync_whats_new` dereferences `out_status` without a NULL check

**File:** `src/sync/sync_client.c:76, 80`
**Issue:** `*out_status = response.status_code;` is unconditional in both branches. Nothing in `sync_client.h`'s contract comment documents `out_status` as non-nullable, and every current call site happens to pass a valid pointer, but the function will crash if a future caller passes `NULL` (a plausible mistake since `out` and `max_count` already have "best effort" semantics elsewhere in the codebase).
**Fix:** Either document `out_status` as required (non-NULL) in the header comment, or guard the writes: `if (out_status) *out_status = response.status_code;`.

### IN-03: `label` value stored without trimming despite trim-based validation

**File:** `supabase/functions/register-device/index.ts:36-40`
**Issue:** The empty-check uses `body.label.trim() !== ""` but the value actually assigned and persisted is the untrimmed `body.label`. A label submitted with leading/trailing whitespace (e.g., `"ratimos-whats-new-test "`) would pass validation but fail exact-match lookups elsewhere (e.g., the seed script's `where label = 'ratimos-whats-new-test'`), silently breaking the device resolution the seed script depends on.
**Fix:** `label = body.label.trim();`

---

_Reviewed: 2026-08-28T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
