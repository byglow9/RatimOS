-- Phase 2 Plan 2: one-time, manually-run content_items seed (D-06/D-11).
--
-- Deliberately NOT part of any CLI seed/reset convention -- paste and run
-- this script's full contents in the Supabase Dashboard's SQL Editor
-- against the live project, exactly once, after the 'ratimos-whats-new-test'
-- device has been registered via register-device (Plan 1). Re-running this
-- script is safe to skip on future test runs; it must never auto-run.
--
-- Each row resolves device_id via the label subquery below, NEVER a
-- hardcoded UUID literal -- a literal ID would silently break the moment
-- the test device is ever re-registered (a fresh registration mints a new
-- devices row with a new id, same label).
--
-- 3 fictitious rows, one of each type, generic placeholder titles mirroring
-- Phase 1's fixture convention (assets/mock/letters/carta1.txt: "Carta de
-- teste 1"). All left delivered_at NULL -- still pending (D-12); this phase's
-- whats-new endpoint never writes delivered_at.

insert into content_items (device_id, title, type)
values (
  (select id from devices where label = 'ratimos-whats-new-test' order by created_at desc limit 1),
  'Carta de teste 1',
  'letter'
);

insert into content_items (device_id, title, type)
values (
  (select id from devices where label = 'ratimos-whats-new-test' order by created_at desc limit 1),
  'Foto de teste 1',
  'photo'
);

insert into content_items (device_id, title, type)
values (
  (select id from devices where label = 'ratimos-whats-new-test' order by created_at desc limit 1),
  'Faixa de teste 1',
  'music'
);
