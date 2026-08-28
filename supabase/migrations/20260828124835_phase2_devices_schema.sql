-- Phase 2 Plan 1: devices table + RLS default-deny (D-02/D-08/D-09, T-2-06).
--
-- Opaque bearer-token model (D-08): token_hash stores a SHA-256 hex digest of
-- the plaintext token, never the token itself. RLS is enabled with ZERO
-- policies for anon/authenticated below -- this is deliberate default-deny,
-- not an oversight (Postgres denies all access to those roles once RLS is on
-- and no policy grants it; only service_role, used exclusively inside Edge
-- Functions per D-09, bypasses RLS). Do not add a permissive policy here.

create table devices (
  id uuid primary key default gen_random_uuid(),
  token_hash text not null unique,
  label text,
  created_at timestamptz not null default now(),
  last_seen_at timestamptz
);

alter table devices enable row level security;
-- Intentionally NO policies for anon/authenticated below this line.
