# Deferred Items — Phase 2

Out-of-scope discoveries logged during plan execution (per SCOPE BOUNDARY — not fixed, only recorded).

## From Plan 02-01

- **`src/storage/letters.c:51` — pre-existing `-Wformat-truncation=` warning.** Surfaced during `pio test -e native_sim -f test_sync_integration` (shared build output with the rest of `src/`). Pre-existing from Phase 1 (`ratimos_storage_index_letters`, `snprintf(s_letters[i].title, sizeof(s_letters[i].title), "%s", line)` — `line` can be up to 127 bytes into a 64-byte destination). Out of this plan's file scope (`src/sync/`, `supabase/`, `platformio.ini`, `test/test_sync_integration/`) — not touched.
