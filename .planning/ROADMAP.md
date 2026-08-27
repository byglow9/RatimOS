# Roadmap: RatimOS

## Overview

RatimOS is built as a stack of horizontal technical layers, sequenced by physical hardware dependency and risk rather than as independent end-to-end feature slices. Everything that can be validated on a PC simulator (shell navigation, storage abstraction, sync protocol, backend, security model) comes first, at zero hardware risk. Once the Waveshare ESP32-S3-Touch-LCD-3.5 board arrives, bring-up proceeds in strict dependency order: display/touch (needed to see anything), power/RTC (needed for safety and correct timestamps), then audio/storage/camera (needed for real content), then WiFi provisioning (needed before any sync), then cloud sync, then OTA plus a full concurrent-subsystems stress test. Visual identity and games are lower-risk content work slotted in once the core plumbing is stable. Security hardening (Secure Boot + Flash Encryption) is deliberately the very last phase, since it burns irreversible eFuses and must only happen after OTA rollback has been proven — the device can never be physically retrieved to fix a brick.

## Phases

**Phase Numbering:**

- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Shell, Storage API & Simulator-First App Shells** - Home menu navigation and RatimOS boot identity run end-to-end in the PC simulator, with all 5 apps wired to a shared Storage/Content API (completed 2026-08-27)
- [ ] **Phase 2: Sync Protocol, Security Model & Cloud Backend** - Supabase backend and per-device HTTPS token authentication exist and are provably secure, tested from a PC build
- [ ] **Phase 3: Hardware Bring-Up — Display, Touch, Boot & Partition Scheme** - Firmware runs on the real board for the first time with working display/touch, verified stylus compatibility, and an OTA-ready partition table
- [ ] **Phase 4: Power Management — Battery, PMIC, RTC** - Device runs on its own battery with accurate charge state and correct persistent timekeeping
- [ ] **Phase 5: Audio, Storage & Camera Bring-Up** - Music playback, photo album, and letters work against real on-device storage, with camera capture
- [ ] **Phase 6: WiFi Provisioning & Settings** - User can get the device onto her wifi and control brightness/volume/info entirely from the device, no hardcoded credentials
- [ ] **Phase 7: Cloud Content Sync** - New letters/photos/music sent from the backend arrive on the device and are visibly flagged as new, without breaking offline use
- [ ] **Phase 8: OTA & Concurrent Integration Stress Test** - Firmware updates itself safely over the air with proven rollback, and the device survives every subsystem running together
- [ ] **Phase 9: Visual Identity & Games** - RatimOS has its own bespoke look and 2 complete, playable board/card games
- [ ] **Phase 10: Security Hardening (Secure Boot + Flash Encryption)** - Device is irreversibly locked down before delivery, only after every other system is proven stable

## Phase Details

### Phase 1: Shell, Storage API & Simulator-First App Shells

**Goal**: The RatimOS shell runs end-to-end in the PC simulator — home menu navigation and boot identity work as they will on the real device, and every app is wired to a shared Storage/Content API instead of touching storage directly.
**Depends on**: Nothing (first phase)
**Requirements**: SHELL-01, SHELL-03
**Success Criteria** (what must be TRUE):

  1. Launching the simulator shows a RatimOS boot identity/splash before landing on the home screen (never a stock/blank screen)
  2. From the home screen, user can open any of the 5 sections (jogos/musica/album/cartas/config) and return to home
  3. All 5 app screens read their content exclusively through the shared Storage/Content API (no app code touches SD/NVS/WiFi directly)
  4. The native SDL2 HAL (`board_native_sdl`) mirrors the structure of the eventual real HAL (`board_waveshare_s3_35`), so app code compiles unchanged against both

**Plans**: 3/3 plans executed
Plans:
**Wave 1**

- [x] 01-01-PLAN.md — Board/HAL split, storage core & boot splash (tracer: board+storage+splash+cartas)
- [x] 01-02-PLAN.md — Visual identity palette repaint (D-17) & shared row-list truncation

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 01-03-PLAN.md — Expand storage to all 5 domains, wire remaining 4 apps, final phase-gate UAT

**UI hint**: yes

### Phase 2: Sync Protocol, Security Model & Cloud Backend

**Goal**: The cloud backend and device-authentication protocol exist and are provably secure and testable from a PC build, independent of ESP32 hardware.
**Depends on**: Phase 1
**Requirements**: SEC-01, SEC-02
**Success Criteria** (what must be TRUE):

  1. Supabase schema (devices, content_items, ota_releases) exists with RLS policies scoping each device to only its own data
  2. A PC-native test client authenticates using a per-device bearer token and gets rejected (401) with no token or another device's token
  3. Every backend request/response in the protocol client goes over HTTPS; no endpoint is reachable over plaintext HTTP
  4. A "what's new" endpoint returns pending letters/photos/music for a given device token, verified against the real Supabase project from a PC-native build

**Plans**: 4 plans
Plans:
**Wave 1**

- [ ] 02-01-PLAN.md — Tracer: devices schema+RLS, register-device Edge Function, native_curl HTTP client (D-04/D-05/D-08)

**Wave 2** *(blocked on Wave 1 completion)*

- [ ] 02-02-PLAN.md — content_items+ota_releases schema, whats-new Edge Function, seed data (D-10/D-11/D-12/D-13)
- [ ] 02-03-PLAN.md — sync_client.h/.c (cJSON-backed whats-new client) + offline unit suite

**Wave 3** *(blocked on Wave 2 completion)*

- [ ] 02-04-PLAN.md — Full auth-reject matrix, HTTPS-only guarantee & live RLS proof against the real project

### Phase 3: Hardware Bring-Up — Display, Touch, Boot & Partition Scheme

**Goal**: Firmware runs on the real Waveshare ESP32-S3-Touch-LCD-3.5 board for the first time, with a working display/touch driver, a real boot sequence, and an OTA-ready partition table from the very first flash.
**Depends on**: Phase 1
**Requirements**: INPUT-01, INPUT-02, OTA-03
**Success Criteria** (what must be TRUE):

  1. The RatimOS boot identity and home screen render correctly on the physical device's ST7796 display after power-on
  2. Every screen responds correctly to finger touch on the real FT6336 touch panel, with tap targets accurate after calibration
  3. Passive-stylus compatibility with the capacitive touch panel is verified and documented as confirmed or disproven, with a decision recorded on whether "cartas" UI needs a finger-sized-target redesign
  4. The very first firmware image ever flashed to the device uses a dual-partition (`ota_0`/`ota_1`) table with app rollback enabled, confirmed by inspecting the flashed partition table

**Plans**: TBD
**UI hint**: yes

### Phase 4: Power Management — Battery, PMIC, RTC

**Goal**: The device runs on its own battery with accurate charge state and correct persistent timekeeping, safely.
**Depends on**: Phase 3
**Requirements**: POWER-01, POWER-02, POWER-03, SHELL-02
**Success Criteria** (what must be TRUE):

  1. Device charges via USB-C with charge current/cutoff matched to the battery's datasheet, verified against the AXP2101 configuration
  2. Home screen shows real, live battery percentage and charging state sourced from the PMIC fuel gauge, not a placeholder
  3. Device keeps correct date/time via the PCF85063 RTC across a full power loss and reboot (battery and USB both disconnected, then reconnected)
  4. Home screen shows the current time at all times, sourced from the RTC

**Plans**: TBD
**UI hint**: yes

### Phase 5: Audio, Storage & Camera Bring-Up

**Goal**: Music, photo, and letter content work against real on-device storage, with working audio playback and camera capture.
**Depends on**: Phase 4
**Requirements**: MUSICA-01, MUSICA-02, ALBUM-01, ALBUM-02, CARTAS-01
**Success Criteria** (what must be TRUE):

  1. User can play a local MP3 file with working play/pause/next/previous/volume controls, audible through the ES8311 codec
  2. Music keeps playing in the background while the user browses to other RatimOS apps
  3. User can browse a thumbnail grid of photos stored on the SD card and view any photo full-screen
  4. User can capture a new photo with the onboard OV5640 camera and see it appear in the album
  5. User can browse and read letters stored on the SD card in a legible, paginated reading view

**Plans**: TBD
**UI hint**: yes

### Phase 6: WiFi Provisioning & Settings

**Goal**: The user can get the device onto her wifi network and control its basic settings entirely from the device itself, with no credentials ever hardcoded.
**Depends on**: Phase 5
**Requirements**: CONFIG-01, CONFIG-02, CONFIG-03, CONFIG-04
**Success Criteria** (what must be TRUE):

  1. User can provision wifi (SSID/password) from the device with zero credentials hardcoded in firmware
  2. User can re-provision/change wifi from the device when the network changes or credentials stop working
  3. User can adjust screen brightness and audio volume from the settings screen and see/hear the change immediately
  4. Settings screen shows current firmware version and storage space used

**Plans**: TBD
**UI hint**: yes

### Phase 7: Cloud Content Sync

**Goal**: New letters, photos, and music sent from the backend actually arrive on the device and are visibly flagged as new, without breaking offline use.
**Depends on**: Phase 6
**Requirements**: SYNC-01, SYNC-02, SYNC-03, ALBUM-03, CARTAS-02
**Success Criteria** (what must be TRUE):

  1. Device automatically pulls new letters/photos/music from the cloud backend when connected to wifi, without user action
  2. New synced photos appear in the album alongside camera-captured photos
  3. New synced letters appear in the cartas list with a visible "new" indicator until read
  4. A visible new-content badge/animation appears when unviewed synced content exists
  5. All 5 sections continue to work fully (browse existing content, play games/music) when wifi is turned off or unavailable

**Plans**: TBD
**UI hint**: yes

### Phase 8: OTA & Concurrent Integration Stress Test

**Goal**: Firmware updates itself safely over the air, and the whole device survives every subsystem running together under real conditions.
**Depends on**: Phase 7
**Requirements**: OTA-01, OTA-02
**Success Criteria** (what must be TRUE):

  1. Device checks for and downloads a new firmware version from the backend over HTTPS on request
  2. A deliberately broken OTA update automatically rolls back to the last known-good firmware without bricking the device, proven by flashing a bad build and confirming recovery
  3. Camera capture, wifi sync, display rendering, and SD access running concurrently on battery power do not cause a brownout reset or data corruption, verified with a sustained stress test

**Plans**: TBD

### Phase 9: Visual Identity & Games

**Goal**: RatimOS looks and plays like a finished, bespoke product — its own visual identity and complete, playable games.
**Depends on**: Phase 8 (sequenced after core plumbing to avoid restyling UI mid-flight; only hard technical dependency is Phase 3's proven touch/stylus input)
**Requirements**: VISUAL-01, JOGOS-01, JOGOS-02
**Success Criteria** (what must be TRUE):

  1. RatimOS uses its own color palette, typography, and icon set, visually distinct from the colombiaOS reference project (confirmed by side-by-side comparison, no copied assets)
  2. User can play at least 2 complete, simple board/card games (e.g. sudoku, paciência) fully using touch and stylus input
  3. In-progress game state (e.g. a partially solved sudoku board) is not lost when navigating away to another app and back

**Plans**: TBD
**UI hint**: yes

### Phase 10: Security Hardening (Secure Boot + Flash Encryption)

**Goal**: The gift device is irreversibly locked down before delivery, with no recoverable path back to an unsecured state, and only after every other system has been proven stable.
**Depends on**: Phase 9 (sequenced last; hard technical dependency is Phase 8's proven OTA rollback — Secure Boot/Flash Encryption must never be attempted before rollback is proven)
**Requirements**: SEC-03, SEC-04
**Success Criteria** (what must be TRUE):

  1. Secure Boot v2 and Flash Encryption are enabled on the actual gift device, following a full rehearsal of the enable/verify/OTA cycle on a disposable dev board first
  2. No wifi password, API key, or token is stored in plaintext anywhere in the firmware source or on the SD card, confirmed by inspecting the built binary and SD contents
  3. A final review confirms every device-backend request still uses HTTPS with a valid per-device token after security hardening is applied

**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Shell, Storage API & Simulator-First App Shells | 3/3 | Complete    | 2026-08-27 |
| 2. Sync Protocol, Security Model & Cloud Backend | 0/4 | Not started | - |
| 3. Hardware Bring-Up — Display, Touch, Boot & Partition Scheme | 0/TBD | Not started | - |
| 4. Power Management — Battery, PMIC, RTC | 0/TBD | Not started | - |
| 5. Audio, Storage & Camera Bring-Up | 0/TBD | Not started | - |
| 6. WiFi Provisioning & Settings | 0/TBD | Not started | - |
| 7. Cloud Content Sync | 0/TBD | Not started | - |
| 8. OTA & Concurrent Integration Stress Test | 0/TBD | Not started | - |
| 9. Visual Identity & Games | 0/TBD | Not started | - |
| 10. Security Hardening (Secure Boot + Flash Encryption) | 0/TBD | Not started | - |

---
*Roadmap created: 2026-08-26*
*Granularity: fine (10 phases)*
*Mode: standard (Horizontal Layers) — phases sequenced by hardware/physical dependency, not vertical feature slices*
