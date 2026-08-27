# API Coverage — Supabase (Postgres + Auth-adjacent tokens + Edge Functions + PostgREST)

> Full coverage by default. Opt-outs are explicit, reasoned decisions.

| capability | decision | reason |
|---|---|---|
| Device registration (Edge Function, admin-secret gated) | INTEGRATE | D-06/D-07 — the mechanism Phase 6's BLE provisioning will call later |
| Opaque bearer-token issuance &amp; SHA-256 hashing | INTEGRATE | D-08 — the core auth primitive this phase exists to build |
| Bearer-token validation (Edge Function, per-request) | INTEGRATE | D-09 — the primary access-control path for every device request |
| Content listing / "what's new" metadata endpoint | INTEGRATE | D-10 — required by ROADMAP success criterion #4 |
| Postgres RLS (default-deny, no policies) on devices/content_items/ota_releases | INTEGRATE | D-02/D-09 — required by ROADMAP success criterion #1, the defense-in-depth backstop |
| Database migrations via Supabase CLI (`supabase db push`) | INTEGRATE | Only supported way to apply schema to the real hosted project |
| Edge Function secrets management (`supabase secrets set`) | INTEGRATE | Required to deliver ADMIN_REGISTRATION_SECRET to register-device without hardcoding it |
| Supabase Dashboard SQL Editor (manual content_items seed) | INTEGRATE | D-11 — a fixed, manually-run seed script by explicit decision |
| Storage (file upload/download for photos/music binaries) | OPT-OUT | Deferred to Phase 7 (Cloud Content Sync) per D-10 — this phase is metadata-only |
| Supabase Auth (email/OAuth end-user accounts) | OPT-OUT | Devices authenticate via custom opaque bearer tokens (D-08), not Supabase Auth users — there is no human end-user login in this project |
| Realtime subscriptions | OPT-OUT | Not needed — this project is explicitly pull-based ("device pulls, never receives pushes" per CLAUDE.md's OTA/sync architecture); no push-triggered client behavior exists |
| Database Webhooks | OPT-OUT | No push-triggered side effects needed this phase — same pull-based rationale as Realtime |
| Supabase Vault / column-level encryption | OPT-OUT | The device token is already hashed at rest before storage (D-08); no additional column encryption is needed for this phase's scope |
| Full-text search / advanced Postgres extensions | OPT-OUT | Not needed for a metadata-only, single-device content listing |
| ota_releases data population | OPT-OUT | D-13 — schema-only, zero rows this phase; Phase 8 decides the real shape and populates it |
| Multi-device / multi-tenant support | OPT-OUT | Explicitly out of scope for the entire project (one-off personal gift device, not a multi-tenant product) — see CONTEXT.md Deferred Ideas |

**Second-integration note:** this is the first and only Supabase integration in the project; no prior integration's opt-outs are being carried over.
