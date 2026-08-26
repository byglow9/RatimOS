# Architecture Research

**Domain:** ESP32-S3/LVGL embedded handheld device (local-first firmware + WiFi content sync + minimal cloud backend)
**Researched:** 2026-08-26
**Confidence:** MEDIUM (official ESP-IDF docs for OTA/provisioning/security; community consensus for LVGL app structure and Supabase-as-IoT-backend, no single authoritative "handheld gadget" reference architecture exists — this is a synthesis)

## Standard Architecture

### System Overview

```
┌──────────────────────────── DEVICE (ESP32-S3) ─────────────────────────────┐
│                                                                              │
│  ┌────────────────────────── App Layer ───────────────────────────────┐    │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐            │    │
│  │  │ jogos  │ │ musica │ │ album  │ │ cartas │ │ config │            │    │
│  │  └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘            │    │
│  │      │          │          │          │          │                 │    │
│  └──────┴──────────┴────┬─────┴──────────┴──────────┴─────────────────┘    │
│                          │  (all apps talk ONLY through Shell + Storage API) │
│  ┌───────────────────────┴─────────────────────────────────────────────┐   │
│  │              Shell / Screen Manager (LVGL, lv_scr_load)              │   │
│  │      pre-created screens, app registry, event-driven navigation      │   │
│  └───────────────────────┬─────────────────────────────────────────────┘   │
│                          │                                                  │
│  ┌───────────────────────┴─────────────────────────────────────────────┐   │
│  │                      Storage / Content API                          │   │
│  │   NVS (config/state kv)   │   SD card FAT (photos/music/letters)    │   │
│  └───────────────────────┬─────────────────────────────────────────────┘   │
│                          │                                                  │
│  ┌───────────────────────┴─────────────────────────────────────────────┐   │
│  │                 Sync Client (background task, WiFi-gated)            │   │
│  │  BLE Provisioning │ Content Puller │ OTA Puller │ Device Auth Token  │   │
│  └───────────────────────┬─────────────────────────────────────────────┘   │
└──────────────────────────┼──────────────────────────────────────────────────┘
                            │  HTTPS (device-initiated only)
┌──────────────────────────┴──────────────────────────── CLOUD ─────────────┐
│  ┌──────────────┐   ┌──────────────┐   ┌────────────────┐  ┌───────────┐  │
│  │  Postgres     │   │   Storage     │   │  Auth /         │  │  OTA bin  │  │
│  │ (content meta,│   │ (photo/audio/ │   │  per-device     │  │  server   │  │
│  │  device reg.) │   │  letter files)│   │  token + RLS    │  │(Render/EF)│  │
│  └──────────────┘   └──────────────┘   └────────────────┘  └───────────┘  │
│                         Supabase (Postgres + Storage + Auth + Edge Fns)     │
└──────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| App modules (jogos, musica, album, cartas, config) | Own their screen's UI + interaction logic only; read/write content through the Storage API, never touch SD/NVS/WiFi directly | One `.c/.h` (or `.cpp/.h`) pair per app, each exposing `app_x_create()`/`app_x_destroy()`, registered with the Shell |
| Shell / Screen Manager | Owns navigation, the home menu, transitions between apps, top-level input routing | LVGL `lv_display_t` layers; screens pre-created at boot per LVGL's own multi-screen guidance (avoids stutter from late `lv_obj_create`) |
| Storage / Content API | Single point of truth for "what content exists locally"; abstracts NVS vs SD/LittleFS so apps don't care where a byte lives | Thin C API: `content_list()`, `content_get_path()`, `content_mark_synced()`, backed by NVS for small structured state (sync cursor, last-OTA version, device id) and SD FAT for binary assets |
| Sync Client | Background task that, when WiFi is up, pulls new content and OTA manifests from the backend using the device token, writes into local Storage, never blocks UI apps | FreeRTOS task with a WiFi-connected event trigger + periodic timer; uses `esp_http_client`/`esp_https_ota` |
| BLE Provisioning | One-time (or reset-triggered) flow to get WiFi credentials onto the device without hardcoding them | ESP-IDF `wifi_provisioning` component (`wifi_prov_mgr_*`), credentials land in NVS once done, BLE tears itself down |
| Cloud backend (Supabase) | Source of truth for new content + device registry + auth; issues signed URLs / serves metadata that the device pulls | Postgres tables (`devices`, `content_items`, `ota_releases`) + Storage buckets + RLS scoping each device token to its own rows; optional Edge Function to mint signed URLs or build the "what's new" response |
| OTA binary host | Serves the actual firmware `.bin` over HTTPS for `esp_https_ota` to stream | Either Supabase Storage (signed URL) or a tiny endpoint on Render free tier — a static-file host is enough, no custom logic required |

## Recommended Project Structure

```
firmware/
├── platformio.ini          # multiple envs: esp32-s3 (real board), native (SDL2 sim)
├── src/
│   ├── main.cpp             # boot sequence: storage init -> shell init -> sync task spawn
│   ├── shell/                # screen manager: home menu, transitions, app registry
│   │   ├── shell.c/.h
│   │   └── app_registry.c/.h
│   ├── apps/
│   │   ├── jogos/
│   │   ├── musica/
│   │   ├── album/
│   │   ├── cartas/
│   │   └── config/
│   ├── storage/               # Storage/Content API — the only thing apps call
│   │   ├── nvs_store.c/.h      # config, sync cursor, device id, last OTA version
│   │   ├── sd_store.c/.h        # FAT/SD file access for media
│   │   └── content_api.c/.h      # unified interface apps consume
│   ├── sync/
│   │   ├── wifi_provisioning.c/.h  # BLE prov wrapper around wifi_provisioning component
│   │   ├── content_sync.c/.h        # pull new letters/photos/music
│   │   └── ota_sync.c/.h              # pull + apply firmware via esp_https_ota
│   ├── hal/                    # board-specific: display driver, touch, RTC, audio codec, camera, battery/PMIC
│   │   ├── board_waveshare_s3_35.c/.h
│   │   └── board_native_sdl.c/.h    # simulator stand-ins for the above
│   └── lv_conf.h
├── data/                       # bundled assets flashed to SD/LittleFS image
└── test/                       # unit tests runnable under the `native` env
backend/
├── supabase/                   # migrations, RLS policies, edge functions
│   ├── migrations/
│   └── functions/
└── ota-server/                  # optional separate static host for OTA .bin (Render free tier)
```

### Structure Rationale

- **`apps/` is isolated from `hal/` and `sync/`:** apps must compile and run identically against the `native` (SDL2) HAL and the real board HAL. This is what makes hardware-optional development possible — an app that reaches into a GPIO or SD driver directly breaks the simulator build.
- **`storage/content_api.c` is the seam that makes sync invisible to apps:** the album app just asks "what photos exist," and doesn't know or care whether a photo arrived via camera capture or via a background sync pull. This also means content_sync writes land in the same place camera/local writes do — no separate "synced content" vs "local content" code paths in the UI.
- **`sync/` is a boundary that only talks HTTPS out and Storage API in** — it never touches LVGL, never blocks the UI task. Keeping it a separate FreeRTOS task (not called from the UI loop) is what prevents a stalled network call from freezing the screen.
- **`hal/board_native_sdl.c` mirrors `hal/board_waveshare_s3_35.c`:** this is the actual mechanism that makes the project "simulator-testable first" — every board-specific driver needs a native stand-in (even if it's a stub for camera/RTC/battery) so app + shell + storage logic can be built and demoed on a PC before any hardware purchase.

## Architectural Patterns

### Pattern 1: Local-first Storage with a Single Content API

**What:** Every app calls a small `content_api` (list/get/put/mark_synced) instead of touching NVS or SD directly. NVS holds only small structured state (sync cursor per content type, device id/token, last applied OTA version, user settings). SD/LittleFS holds all binary assets (photos, audio, letter text/images).
**When to use:** Any device that must work fully offline and also accept remotely-pushed content — the API is what lets "new content arrived via sync" and "content is just there" collapse into one code path for the UI.
**Trade-offs:** Slightly more upfront plumbing (one more indirection layer) vs. apps hitting storage directly; pays off the moment sync or camera-writes need to coexist with manually-loaded SD content.

### Pattern 2: Device-Initiated Pull Sync (not push)

**What:** The device — not the backend — decides when to check for new content/OTA. On WiFi-connected events (or a periodic timer while charging/idle), the sync task calls a REST endpoint with its per-device bearer token, gets back "what's new," downloads deltas into local storage.
**When to use:** Intermittently-connected, battery-powered, single-owner devices without a public reachable address — exactly this device's profile. Avoids needing any inbound connectivity, NAT traversal, or a persistent open socket (MQTT/websocket), which is unnecessary complexity for a device that's fine checking in once in a while.
**Trade-offs:** Not real-time (new content might sit for minutes/hours before the device notices) — acceptable here since this isn't a live chat, it's letters/photos/music. Simpler and more battery-friendly than a persistent connection.

### Pattern 3: Dual-Slot OTA over HTTPS (esp_https_ota)

**What:** ESP-IDF's `esp_https_ota` writes new firmware into the inactive OTA partition while running from the active one; only flips `otadata` to boot the new slot after the image validates; the bootloader auto-rolls-back if the new image never marks itself valid.
**When to use:** Any remote-firmware-update requirement — this is Espressif's standard, no need to hand-roll a bootloader OTA scheme.
**Trade-offs:** Needs partition table with 2 app slots sized for the firmware (plan flash size accordingly — a 3.5" LVGL UI app with camera/audio may not be tiny); needs the same device HTTPS+token infra as content sync, so it's natural to route it through the same `sync/` layer with its own puller.

## Data Flow

### Content Sync Flow (cartas/fotos/música → device)

```
[Backend: new content inserted (Postgres row + Storage file)]
    ↓
[Device: WiFi connects] → [Sync task wakes] → [GET /device/{id}/whats-new (token)]
    ↓                                                  ↓
[Backend: RLS-scoped query] → [returns list of new content_items + signed Storage URLs]
    ↓
[Device: for each new item] → [download file via signed URL] → [write to SD via content_api]
    ↓
[content_api marks item synced, advances cursor in NVS]
    ↓
[album/musica/cartas apps see new content next time they list() — no special-case code]
```

### OTA Flow (firmware → device)

```
[Dev builds new firmware.bin] → [uploads to OTA host (Supabase Storage or Render static)]
    ↓
[Backend: ota_releases row created (version, url, checksum)]
    ↓
[Device: sync task's OTA check] → [GET /device/{id}/ota-check (token)] → [compares version to running]
    ↓ (if newer)
[esp_https_ota streams .bin into inactive partition] → [validates] → [otadata flips] → [reboot]
    ↓ (if new image fails self-check)
[bootloader auto-rolls-back to previous partition, no brick]
```

### WiFi Provisioning Flow (one-time, or on reset)

```
[Device boots with no saved WiFi creds] → [starts BLE provisioning service, advertises]
    ↓
[Companion phone app connects over BLE] → [sends SSID + password over protocomm session]
    ↓
[Device joins WiFi] → [credentials saved to NVS] → [BLE service auto-stops]
    ↓
[Sync task can now run on every subsequent boot without any further provisioning step]
```

## Scaling Considerations

This is a single-device, single-owner gift, not a multi-tenant product — "scaling" here means "does the architecture survive going from 0 devices to a couple more later," not user growth.

| Scale | Architecture Adjustments |
|-------|--------------------------|
| 1 device (this project) | Everything above as-is: one `devices` row, one token, Postgres/Storage free tier is overkill-capacity |
| A handful of devices (future gifts) | Same schema already supports it (`devices` table + RLS scoping by device id) — no architecture change needed, just more rows |
| Public/many-user product | Would need real device provisioning at scale (factory-programmed tokens, fleet OTA staged rollout, rate limiting) — explicitly out of scope, don't build for it now |

### Scaling Priorities

1. **Not applicable at this scale.** The one real constraint is Supabase free tier storage/bandwidth caps if photo/music libraries grow large — worth checking free-tier limits before assuming unlimited headroom, but not an architecture concern, a plan/quota concern.
2. **Flash size on-device** is the actual near-term constraint: LVGL UI + 5 apps + camera driver + audio codec + dual OTA partitions all competing for the board's flash. Partition table sizing should be revisited once real firmware size is known from hardware phases.

## Anti-Patterns

### Anti-Pattern 1: Apps Reaching Directly into SD/NVS/WiFi

**What people do:** Let each app (`musica`, `album`, etc.) call SD/FatFS or NVS APIs directly, or check WiFi status itself.
**Why it's wrong:** Couples every app to the real board's storage/network stack, which is exactly what breaks the "simulator-first" build strategy this project needs — the native SDL2 environment has no real SD card or WiFi. It also means sync-written content and locally-created content (e.g. camera photos) can silently diverge in how they're read.
**Do this instead:** All apps go through `content_api`; only `storage/` and `sync/` know about SD/NVS/WiFi/BLE.

### Anti-Pattern 2: Sync/OTA Running on the UI Task

**What people do:** Call the HTTP client or `esp_https_ota` directly from the same FreeRTOS task/loop that runs LVGL's `lv_timer_handler`.
**Why it's wrong:** A stalled DNS lookup or slow download freezes the UI — bad for a device meant to feel like a polished personal gadget, not a device that hangs whenever WiFi is flaky.
**Do this instead:** Sync/OTA run in a dedicated background task; the UI only reads a lightweight status flag (syncing/idle/error) to show a small indicator, never blocks on the network call itself.

### Anti-Pattern 3: Treating Secure Boot/Flash Encryption as a Phase-0 Concern

**What people do:** Try to enable Secure Boot + Flash Encryption from the very first firmware build.
**Why it's wrong:** Both are backed by eFuse burns that are largely irreversible on production hardware, and they increase bootloader size (may force partition table changes) — enabling them early, before the partition layout and OTA flow are settled, means redoing security setup repeatedly and risking burning eFuses on a still-changing image.
**Do this instead:** Build and iterate firmware with security features off; enable Secure Boot + Flash Encryption as a dedicated near-the-end phase once app/storage/sync/OTA are stable, per the project's own stated requirement ("Segurança básica... antes da entrega").

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Supabase Postgres | Device calls a thin REST/Edge Function endpoint (not raw Postgres wire protocol) with its bearer token; endpoint queries RLS-scoped rows | Keeps the ESP32 HTTP client simple (plain HTTPS+JSON) instead of needing a Postgres client library on-device |
| Supabase Storage | Backend issues short-lived signed URLs for photos/audio/letters/OTA binaries; device just does plain HTTPS GET | Avoids embedding Supabase service keys on the device — device only ever holds its own scoped token |
| OTA binary host (Render free tier or Supabase Storage) | `esp_https_ota` streams the `.bin` directly from a URL | A static file host is sufficient; no custom OTA server logic needed beyond serving files + a version-check JSON endpoint |
| BLE provisioning companion app | Espressif's own generic "BLE Provisioning" mobile app works against the standard `wifi_provisioning` component without writing a custom phone app | Removes the need to build/maintain a companion app just for WiFi setup |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Apps ↔ Shell | LVGL screen lifecycle + events (`app_x_create/destroy`, enter/exit callbacks) | Shell owns which screen is active; apps never call `lv_scr_load` on themselves |
| Apps ↔ Storage/Content API | Direct C function calls (`content_list`, `content_get_path`, ...) | Synchronous, local-only — no networking awareness in this boundary |
| Sync Client ↔ Storage/Content API | Same content_api, called from the background task | Sync writes look identical to any other content write; this is what keeps apps sync-agnostic |
| Sync Client ↔ Cloud Backend | HTTPS + per-device bearer token, JSON responses, signed URLs for binaries | Device-initiated only; backend never pushes to the device |
| HAL ↔ Everything above | Board driver interface (display/touch/RTC/audio/camera/battery), swapped wholesale between `board_waveshare_s3_35` and `board_native_sdl` | This swap is the mechanism enabling PC-simulator development before hardware exists |

## Build Order Implications (Simulator-First vs Hardware-Required)

Given no hardware exists yet and camera/battery/RTC/audio genuinely need the real board to validate, the dependency graph strongly favors this order:

**Fully simulator-testable (native/SDL2 env, no hardware needed):**
1. Shell/screen manager + app registry + navigation between the 5 screens (already partially done per PROJECT.md's Fase 0)
2. Content API + storage abstraction, backed by a native filesystem stand-in for SD, and an in-memory or file-backed stand-in for NVS
3. Each app's UI/interaction logic (jogos board games, música/album/cartas browsing UI) against fixture/mock content
4. Sync Client's *protocol* logic (HTTP client calls, JSON parsing, token handling, "what's new" diffing) — can run against the real Supabase backend or a local mock server entirely from a PC build, since HTTPS works the same from `native` PlatformIO env as from the ESP32
5. Cloud backend itself (Supabase schema, RLS policies, Edge Functions, OTA host) — entirely independent of firmware, can be built/tested in parallel any time

**Requires real hardware to validate (but code can still be structured/stubbed earlier):**
6. Display/touch driver bring-up (QSPI panel + touch controller specifics) — first thing needed once the board arrives, blocks all visual on-device verification
7. BLE provisioning flow (real BLE radio + real phone pairing)
8. RTC (PCF85063) — persistent clock, needed for "cartas" timestamps/scheduling to be meaningful for real
9. Audio codec (ES8311) — música app's actual playback
10. Camera (OV5640 via LCD_CAM/GDMA) — highest-complexity hardware bring-up per research (DMA-driven capture, GPIO pin planning)
11. Battery/PMIC (AXP2101) — charge/discharge management, cannot be meaningfully tested off real battery hardware
12. Real OTA end-to-end (esp_https_ota against the real dual-partition table on real flash) — the *client logic* can be unit-tested earlier, but the actual flash-and-reboot cycle needs the board
13. Secure Boot + Flash Encryption — deliberately last (per Anti-Pattern 3 above), after partition table and OTA flow are proven stable, since eFuse changes are largely irreversible

**Practical sequencing:** everything in the first list can and should be built (and demoed to the user, if desired) entirely on the developer's PC before any hardware purchase — this validates 90% of the "does this feel like a good gift" question (navigation, app UX, content browsing, sync-arrives-content behavior) with zero hardware risk. The moment the board arrives, work shifts to bring-up items 6-13 roughly in that order, since display+touch is a hard prerequisite for visually confirming anything else, and Secure Boot/Flash Encryption should stay a final pre-delivery step, not a running concern throughout development.

## Sources

- [PlatformIO — LVGL 9.5 documentation](https://lvgl.io/docs/open/9.5/integration/frameworks/platformio) — MEDIUM
- [lv_platformio (official LVGL PlatformIO template)](https://github.com/lvgl/lv_platformio) — MEDIUM
- [Display and Screen Management | lvgl/lvgl | DeepWiki](https://deepwiki.com/lvgl/lvgl/3.2-display-and-screen-management) — LOW
- [LVGL handle multiple screens (forum)](https://rntlab.com/question/lvgl-handle-multiple-screens/) — LOW
- [Non-Volatile Storage Library — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html) — MEDIUM
- [Over The Air Updates (OTA) — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html) — MEDIUM
- [Wi-Fi Provisioning — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v4.3.6/esp32/api-reference/provisioning/wifi_provisioning.html) — MEDIUM
- [Security Overview — ESP32-S3 — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/security.html) — MEDIUM
- [Secure Boot v2 — ESP32-S3 — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/secure-boot-v2.html) — MEDIUM
- [Flash Encryption — ESP32-S3 — ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/security/flash-encryption.html) — MEDIUM
- [Camera Application — ESP-FAQ](https://docs.espressif.com/projects/esp-faq/en/latest/application-solution/camera-application.html) — MEDIUM
- [Integration of OV5640 camera into ESP32-S3 using GDMA and LCD_CAM (ESP32 Forum)](https://www.esp32.com/viewtopic.php?t=33510) — LOW
- [Building Lean IoT Startups with Supabase — Part 2 (Medium)](https://medium.com/@adithyavenkatesh/building-lean-iot-startups-with-supabase-part-2-designing-a-real-iot-backend-without-the-bloat-d08f7b9e2f56) — LOW
- [Edge Functions Architecture | Supabase Docs](https://supabase.com/docs/guides/functions/architecture) — MEDIUM
- [Send cloud-to-device messages — Azure IoT Hub (polling pattern reference)](https://learn.microsoft.com/en-us/azure/iot-hub/how-to-cloud-to-device-messaging) — LOW

---
*Architecture research for: ESP32-S3/LVGL embedded handheld personal device (RatimOS)*
*Researched: 2026-08-26*
