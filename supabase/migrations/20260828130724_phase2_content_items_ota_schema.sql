-- Phase 2 Plan 2: content_items + ota_releases tables + RLS default-deny
-- (D-09/D-10/D-12/D-13, T-2-06).
--
-- Same "RLS locked-but-empty" posture as the devices migration (Plan 1):
-- enable RLS, add ZERO policies for anon/authenticated. Only service_role
-- (used exclusively inside Edge Functions per D-09) is meant to reach these
-- tables at all -- a raw PostgREST call using only the publishable key must
-- get an empty result set, never real rows.
--
-- content_items.delivered_at is nullable; null = still pending (D-12). This
-- phase's whats-new Edge Function only ever READS this column -- it never
-- writes it. Marking a row delivered is explicitly Phase 7's job, once local
-- persistence can confirm the sync actually succeeded (RESEARCH.md Open
-- Question 2).
create table content_items (
  id uuid primary key default gen_random_uuid(),
  device_id uuid not null references devices(id) on delete cascade,
  title text not null,
  type text not null check (type in ('letter', 'photo', 'music')),
  content_date date not null default current_date,
  url text,
  delivered_at timestamptz,
  created_at timestamptz not null default now()
);

alter table content_items enable row level security;
-- Intentionally NO policies for anon/authenticated below this line.

-- ota_releases (D-13): column structure only, zero rows seeded this phase --
-- Phase 8 decides the real shape (rollback tracking fields, etc.) later.
create table ota_releases (
  id uuid primary key default gen_random_uuid(),
  version text not null,
  url text,
  released_at timestamptz
);

alter table ota_releases enable row level security;
-- Intentionally NO policies for anon/authenticated below this line.

-- Rule 2 auto-fix (anticipated from Plan 1's identical finding on `devices`):
-- this project's "Automatically expose new tables" setting is disabled
-- (D-02), which also suppresses Postgres's default table-level GRANT to
-- service_role on newly created tables. Without this explicit grant,
-- whats-new's service-role client would hit the same 42501
-- permission-denied error Plan 1 hit on `devices`, before ever reaching the
-- RLS check. anon/authenticated remain ungranted -- default-deny (T-2-06)
-- is unchanged.
grant select, insert, update, delete on content_items to service_role;
grant select, insert, update, delete on ota_releases to service_role;
