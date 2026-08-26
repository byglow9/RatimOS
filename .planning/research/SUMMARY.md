# Project Research Summary

**Project:** RatimOS
**Domain:** ESP32-S3 + LVGL handheld "retro OS" personal gift device (firmware + lightweight cloud backend)
**Researched:** 2026-08-26
**Confidence:** MEDIUM

## Executive Summary

RatimOS sits at the intersection of three established DIY hobbyist genres — ESP32 retro handhelds, gift/message devices (Lovebox-style), and LVGL custom-UI embedded products. Experts building this kind of device use PlatformIO + Arduino-core-for-ESP32 (via the `pioarduino` platform fork) with LVGL v9 for UI, LovyanGFX for the ST7796+FT6336 display/touch stack, dedicated Arduino-ecosystem libraries for audio (AudioTools/ES8311) and camera (esp32-camera/OV5640), and a lightweight managed backend (Supabase, optionally paired with Cloudflare R2 for bulk media) rather than self-hosted infrastructure. The architecture that best fits this domain is local-first: a Storage/Content API is the single seam through which all five apps (jogos/musica/album/cartas/config) read and write, so that content arriving via background sync, camera capture, or manual SD load is indistinguishable to the UI. Sync and OTA are strictly device-initiated pull operations over HTTPS with a per-device bearer token — never push, never a persistent connection — which matches the device's intermittent-connectivity, single-owner, battery-powered profile and sidesteps a whole class of infrastructure the solo hobbyist developer doesn't need to run.

The recommended approach is heavily simulator-first: nearly everything except display/touch bring-up, BLE provisioning, RTC, audio, camera, battery/PMIC, and real OTA/security can be built and validated on a PC in the existing SDL2 environment before hardware ever arrives, provided app code never reaches directly into SD/NVS/WiFi (an anti-pattern the architecture research flags explicitly). This lets 90% of the "does this feel like a good gift" question — navigation, UX, content browsing, sync-arrives-content behavior — be answered with zero hardware risk.

The key risks are not primarily software bugs but a cluster of irreversible or safety-critical mistakes concentrated in the hardware phases: (1) the capacitive touch panel may not reliably sense a passive stylus, directly threatening the "cartas" handwriting UX and needing verification before any stylus-dependent UI is built; (2) LiPo battery/PMIC misconfiguration is a genuine fire/safety risk, not just a bug; (3) OTA without a proven, deliberately-tested rollback path can permanently brick a device the developer has explicitly committed to never physically retrieving; and (4) Secure Boot + Flash Encryption burn one-way eFuses and must be rehearsed on disposable hardware and sequenced strictly after OTA rollback is proven — never attempted directly on the gift device. Mitigation across all of these is the same pattern: sequence irreversible/safety-critical operations last, rehearse on disposable/bench setups first, and build explicit self-test/rollback/recovery paths rather than assuming the happy path.

## Key Findings

### Recommended Stack

The firmware stack builds on the project's existing choices (PlatformIO + Arduino-esp32 + LVGL v9, SDL2 simulator) and fills in the rest with the de-facto Arduino-ecosystem standard for each subsystem, since ESP-ADF and raw ESP-IDF component APIs are considered too heavyweight for a first-time embedded solo developer. The backend is a managed, near-zero-cost SaaS (Supabase) rather than self-hosted infrastructure, directly satisfying the constraint that the developer cannot rely on keeping his own machine alive.

**Core technologies:**
- `pioarduino/platform-espressif32` fork (not the official platform) — tracks Espressif's Arduino core releases far faster; pin an explicit release tag for reproducibility
- LVGL 9.5.x + LovyanGFX — UI toolkit already chosen; LovyanGFX drives the ST7796 display + FT6336 touch with DMA-based SPI flush, less boilerplate than raw `esp_lcd`
- `pschatzmann/arduino-audio-tools` + `arduino-audio-driver` + `arduino-libhelix` — I2S/ES8311 audio pipeline and MP3 decode, the standard Arduino-ecosystem alternative to full ESP-ADF
- `bitbank2/JPEGDEC` (display decode) + `espressif/esp32-camera`/`esp_jpeg` (OV5640 capture/encode) — fastest common JPEG path for gallery UX and camera-to-SD pipeline
- `SD_MMC` (4-bit SDMMC, not SD-over-SPI, if pins allow) + NVS/Preferences — media on FAT32 SD card, small structured state (device token, sync cursor) in internal NVS, never credentials on removable media
- ESP-IDF `wifi_provisioning` (BLE) — no hardcoded WiFi credentials, satisfies an Active project requirement
- `HTTPUpdate`/`esp_https_ota` over HTTPS + custom dual-partition (`ota_0`/`ota_1`) table — pull-based, rollback-safe OTA
- Supabase (Postgres + Storage + Auth + Edge Functions), optionally + Cloudflare R2 for bulk media — managed backend fitting the "solo hobbyist, near-zero budget, no ops" constraint
- ESP32-S3 Secure Boot v2 + Flash Encryption — required pre-delivery, but flagged as needing dedicated late-phase, careful handling due to PlatformIO/Arduino tooling friction

### Expected Features

**Must have (table stakes):**
- Home menu with 5 icons + reliable navigation; battery indicator; persistent RTC clock
- Touch and stylus input that actually works
- Fully offline operation of all 5 sections
- Photo album (browse synced photos), letters reading UI, music player with basic transport controls
- 1-2 complete, simple board/card games (no emulation)
- Settings screen with WiFi provisioning (no hardcoded creds)
- New-content indicator ("you have new letters/photos") — the emotional payoff of the sync architecture
- OTA that cannot brick the device (must be baked into the very first shipped firmware)

**Should have (competitive/differentiators):**
- Ongoing remote content sync (letters/photos/music) after delivery — the single feature separating RatimOS from static "make once, give once" gift devices
- Camera captures feeding directly into the album (two-directional, not just a slideshow)
- Fully bespoke visual identity, distinct from the colombiaOS reference
- Real device security (Secure Boot + Flash Encryption + per-device token) — unusual rigor for this hobbyist genre
- Device stays useful/maintained indefinitely via OTA (living product, not sealed)

**Defer (v2+/anti-features — explicitly do not build):**
- Console emulation (NES/GBA/SMS) — bottomless complexity pit, already correctly excluded
- Custom PCB design, companion mobile app, on-device content authoring, real-time push/chat, voice assistant/AI personality, multi-user accounts, elaborate theme engine

### Architecture Approach

A local-first firmware built around a single Storage/Content API that all five apps go through (never touching SD/NVS/WiFi directly), a Shell/Screen Manager owning navigation, and a dedicated background Sync Client task (never on the UI task) that pulls content and OTA updates over HTTPS with a per-device bearer token. The cloud side is Supabase (Postgres for metadata/device registry, Storage for binaries, Auth/RLS for per-device scoping) queried via thin REST/Edge Function endpoints, never raw Postgres wire protocol from the device.

**Major components:**
1. App modules (jogos/musica/album/cartas/config) — own UI/interaction only, read/write exclusively through the Storage/Content API
2. Shell / Screen Manager — home menu, navigation, top-level input routing, pre-created LVGL screens
3. Storage / Content API — single point of truth for "what content exists locally," abstracts NVS vs SD so sync-written and locally-created content are indistinguishable
4. Sync Client — dedicated FreeRTOS task; BLE provisioning, content puller, OTA puller, device auth token; never blocks the UI
5. HAL (board driver layer) — swaps wholesale between `board_waveshare_s3_35` and `board_native_sdl`, the mechanism enabling PC-simulator-first development

### Critical Pitfalls

1. **Capacitive touch may not reliably sense a passive stylus** — verify with a capacitive-tip stylus on real hardware before building any stylus-dependent "cartas" UI; do not assume compatibility.
2. **LiPo battery/PMIC misconfiguration is a fire/safety risk**, not just a bug — use the AXP2101's built-in charge management and fuel gauge, match charge current to the actual cell's rating, never leave a charging cell unattended during initial bring-up.
3. **OTA without a proven rollback path can permanently brick a device the developer can't physically retrieve** — enable `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`, implement a genuine post-update self-test, and prove rollback by deliberately flashing broken firmware before ever shipping OTA.
4. **Secure Boot + Flash Encryption are irreversible eFuse burns** — rehearse the full enable/verify/OTA cycle on a disposable dev board first, and sequence this strictly after OTA rollback is proven, as the very last pre-delivery phase.
5. **Shared QSPI/PSRAM bandwidth and shared SPI bus (display+SD) cause contention** between camera, display, SD, and WiFi — enable PSRAM DMA for camera explicitly, mutex all SPI transactions between display and SD, and treat "camera + wifi sync + display running together" as its own integration checkpoint, not an assumed byproduct of individual feature phases.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Shell, Storage API & Simulator-First App Shells
**Rationale:** Everything not requiring real hardware should be built first — this is the highest-leverage phase since it validates 90% of "does this feel like a good gift" (navigation, UX, content model) with zero hardware risk, and establishes the app/HAL boundary that makes the rest of the project simulator-testable.
**Delivers:** Shell/Screen Manager, home menu navigation across 5 sections, Storage/Content API (native filesystem + in-memory NVS stand-ins), each app's UI/interaction logic against fixture content, `board_native_sdl` HAL stubs mirroring the eventual real HAL.
**Addresses:** Home menu, navigation, offline-first operation, photo/letters/music browsing UI (against fixtures)
**Avoids:** Anti-Pattern "apps reaching directly into SD/NVS/WiFi" (Architecture) and Pitfall 9 (simulator false confidence) — by keeping the HAL boundary strict from day one

### Phase 2: Sync Protocol & Cloud Backend (simulator/PC-testable)
**Rationale:** Sync Client's *protocol* logic (HTTP client, JSON parsing, token handling, diffing) and the Supabase backend (schema, RLS, Edge Functions, OTA host) are entirely independent of firmware hardware and can be built/tested from a PC build against the real backend, in parallel with or right after Phase 1.
**Delivers:** Supabase schema (devices, content_items, ota_releases), RLS policies, "what's new" endpoint, device-initiated pull-sync client logic, dual-partition-aware version-check logic (protocol only).
**Uses:** Supabase (Postgres + Storage + Auth + Edge Functions), ArduinoJson v7, plain HTTPS + bearer token pattern
**Implements:** Sync Client component, Content Sync Flow, OTA manifest check flow (protocol-level)

### Phase 3: Hardware Bring-Up — Display, Touch, PSRAM Reality Check
**Rationale:** The first hardware-arrival phase must be a dedicated bring-up/reality-check before resuming feature work, since simulator-validated UI/timing assumptions (frame buffer sizing, touch mapping) do not transfer automatically to real PSRAM/QSPI/touch hardware.
**Delivers:** Working display+touch driver (LovyanGFX/ST7796+FT6336) on real hardware, calibrated touch, verified PSRAM speed/mode, confirmed (or disproven) stylus compatibility.
**Addresses:** Touch+stylus input (table stakes)
**Avoids:** Pitfall 1 (stylus incompatibility) and Pitfall 9 (simulator-to-hardware gap) — first thing verified once hardware exists, before any stylus-dependent UI ships

### Phase 4: Power Management — Battery, PMIC, RTC
**Rationale:** Battery/PMIC work carries actual physical safety risk (not just a software bug) and should be done deliberately early and cautiously, separate from feature-race pressure; RTC is also needed early since letters/photos need correct timestamps.
**Delivers:** AXP2101 driver integration (charge current/cutoff matched to the actual cell), real fuel-gauge battery percentage, PCF85063 RTC read/write, battery indicator UI wired to real hardware.
**Addresses:** Battery indicator, RTC clock (table stakes)
**Avoids:** Pitfall 3 (LiPo safety) — charge attended, on non-flammable surface, matched to cell datasheet from the start

### Phase 5: Audio, Photo Album Storage & Camera Bring-Up
**Rationale:** These three peripherals (ES8311 audio, SD storage, OV5640 camera) each need individual bring-up before being combined; camera is explicitly the highest-complexity hardware bring-up per research (DMA-driven capture, GPIO planning), so sequence it after audio/storage are solid.
**Delivers:** Working music playback (MP3 via AudioTools/libhelix), SD_MMC storage with power-loss-safe write patterns (write-to-temp-then-rename) and SPI mutex vs. display, camera capture → JPEG → album pipeline.
**Addresses:** Music playback, photo album, camera capture → album (differentiator)
**Avoids:** Pitfall 6 (SD corruption), Pitfall 2 (PSRAM/bus contention) — tested in isolation first, per-subsystem, before combining

### Phase 6: WiFi Provisioning, OTA & Concurrent-Subsystems Integration
**Rationale:** Connects the protocol logic from Phase 2 to real hardware; must include the full OTA rollback proof and the explicit "everything running together" stress test (camera + wifi sync + display), since this combination is the single biggest source of contention/brownout bugs per Pitfalls research.
**Delivers:** BLE WiFi provisioning with on-device re-provision UI ("Trocar Wi-Fi") and wrong-credential retry handling, working dual-slot OTA over HTTPS with proven, deliberately-tested rollback, integration stress test of camera+wifi+display+SD running concurrently on battery power.
**Addresses:** WiFi provisioning, OTA updates, new-content indicator, cloud content sync (end-to-end)
**Avoids:** Pitfall 4 (OTA bricking), Pitfall 7 (brownout resets), Pitfall 8 (provisioning lockout) — all three require deliberate adverse-path testing, not just happy-path

### Phase 7: Visual Identity, Games, and Polish
**Rationale:** Bespoke visual identity and the board/card games are largely independent, lower-risk work that can slot in whenever convenient relative to the hardware-bring-up spine above — no hard dependency forces this later, but sequencing it after core plumbing avoids restyling UI that's still being architecturally reshuffled.
**Delivers:** Final palette/typography/icon set distinct from the colombiaOS reference, 1-2 fully playable board/card games (sudoku, paciência).
**Addresses:** Authorial visual identity, games (table stakes + differentiator)

### Phase 8: Security Hardening (Secure Boot + Flash Encryption) — Final Pre-Delivery Phase
**Rationale:** Deliberately last. Both operations burn irreversible eFuses and remove standard USB reflash recovery; must only happen once OTA rollback (Phase 6) and all app/storage/sync logic are fully proven stable, and only after a full rehearsal on disposable hardware.
**Delivers:** Secure Boot v2 + Flash Encryption enabled on the actual gift device, per-device HTTPS bearer token enforced server-side, final security review against the project's Active security requirement.
**Avoids:** Pitfall 5 (irreversible bricking) — full enable/verify/OTA-update cycle rehearsed on a throwaway ESP32-S3 board before ever touching the gift hardware

### Phase Ordering Rationale

- Simulator-testable work (Phases 1-2) is sequenced first because it has zero hardware risk and validates the majority of the "is this a good gift" question before any hardware purchase risk is incurred.
- Hardware bring-up phases (3-6) follow the dependency order surfaced in Architecture research: display+touch is a hard prerequisite for visually confirming anything else, so it comes immediately after simulator work; battery/PMIC is sequenced early both because RTC/battery UI are table stakes and because it's the one phase with genuine physical safety stakes worth doing without feature-delivery pressure.
- Camera is deliberately the last "isolated" peripheral bring-up (Phase 5) because it's flagged as the highest-complexity hardware integration and benefits from audio/storage patterns being already proven.
- OTA + provisioning + the full concurrent-subsystems stress test are grouped into one phase (6) because Pitfalls research explicitly calls out "camera + wifi sync + display running together" as needing its own dedicated integration milestone, not an assumed byproduct of separate feature phases.
- Security hardening is unconditionally last (Phase 8), directly reflecting the Architecture anti-pattern and Pitfalls guidance that Secure Boot/Flash Encryption are one-way operations that must only happen after every other system (especially OTA rollback) is proven — doing this earlier risks a still-changing firmware image needing repeated eFuse-adjacent rework, or worse, an irreversible brick.

### Research Flags

Needs deeper research during planning (`--research-phase`):
- **Phase 3 (Display/touch bring-up):** exact LVGL v9 driver wiring for this specific Waveshare ST7796+FT6336 combo not yet hands-on-validated; pin mapping needs confirmation against board schematic.
- **Phase 5 (Camera bring-up):** OV5640 DVP pin mapping for this specific Waveshare board not yet independently verified; PSRAM DMA configuration for camera needs board-specific validation.
- **Phase 6 (OTA + provisioning + integration stress test):** concurrent camera+wifi+display+SD bandwidth contention behavior is a reasoned inference from general ESP32-S3 patterns, not tested on this exact board; BLE provisioning wiring from Arduino (vs raw IDF) needs validation.
- **Phase 8 (Security hardening):** PlatformIO+Arduino-framework friction with Secure Boot v2/Flash Encryption tooling is community-reported, not officially documented — needs hands-on validation of the actual workflow (`idf.py`/`espsecure.py` invocation from within a PlatformIO project) before this phase is planned in detail.

Phases with standard, well-documented patterns (skip research-phase):
- **Phase 1 (Shell/Storage/App shells):** LVGL multi-screen patterns and the app/HAL separation are well-established community/official patterns.
- **Phase 2 (Sync protocol/backend):** Supabase + device-initiated pull-sync over HTTPS+token is a standard, well-documented pattern; ArduinoJson v7 usage is straightforward.
- **Phase 4 (Battery/RTC):** XPowersLib (AXP2101) and PCF85063 libraries are documented and purpose-built for this exact chip pairing.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | MEDIUM | Cross-verified via multiple independent web sources and official Espressif/vendor docs, but no hands-on validation on the physical board yet; several rows (LovyanGFX+ST7796/FT6336 compatibility, exact camera pin mapping) explicitly flagged as needing on-hardware confirmation |
| Features | MEDIUM | Strong pattern-matching across three well-established DIY sub-genres (retro handhelds, gift devices, LVGL products), but no single authoritative "gift handheld" spec exists since this is a niche hobbyist category |
| Architecture | MEDIUM | Official ESP-IDF docs for OTA/provisioning/security are solid (MEDIUM-HIGH), but LVGL multi-screen and Supabase-as-IoT-backend guidance rely more on community sources (LOW-MEDIUM); no single authoritative "handheld gadget" reference architecture exists — this is a synthesis |
| Pitfalls | MEDIUM-HIGH | Espressif official docs + ESP-IDF issue tracker + LVGL docs/forum cross-checked against multiple independent hobbyist reports; strongest-sourced of the four research areas, though still no project-specific hands-on validation possible pre-hardware |

**Overall confidence:** MEDIUM

### Gaps to Address

- **Exact Waveshare board pinout (camera DVP header, SDMMC pins, shared SPI bus mapping):** not independently verified from vendor docs (403-blocked wiki fetch relied on search-indexed excerpts) — must be confirmed from the physical board/schematic at the start of Phase 3, before display/touch/camera wiring begins.
- **Stylus compatibility with the FT6336 capacitive touch controller:** genuinely unknown until tested on hardware — flagged as the first thing to verify in Phase 3, since it may force a redesign of the "cartas" handwriting UX around finger-sized targets instead of pen precision.
- **Secure Boot v2 + Flash Encryption workflow under PlatformIO+Arduino specifically (vs raw ESP-IDF):** community-reported friction, not officially documented — needs a dedicated spike/rehearsal on a disposable dev board early enough to inform how Phase 8 is actually planned, not discovered for the first time during that phase.
- **Supabase free-tier limits (500MB DB / 1GB storage / 5GB egress, 7-day auto-pause) and their interaction with device polling frequency:** cross-verified across third-party pricing summaries, not fetched directly from supabase.com this session — re-verify exact current numbers before committing budget/sync-interval assumptions in Phase 2 planning.
- **Concurrent camera+wifi+display+SD bandwidth/brownout behavior on this exact board:** a reasoned inference from general ESP32-S3 architecture, not board-specific data — must be empirically stress-tested as its own explicit milestone in Phase 6, not assumed safe based on individual subsystem tests.

## Sources

### Primary (HIGH confidence)
- LVGL official changelog (lvgl.io/docs) — v9.5.0 current version confirmation
- ESP-IDF OTA, Flash Encryption, Wi-Fi Provisioning, Secure Boot v2, NVS documentation (docs.espressif.com, official) — mechanism and safety-model confirmation
- LVGL Espressif Tips and Tricks (docs.lvgl.io, official) — performance guidance
- ESP-IDF i2s_es8311 example README (espressif/esp-idf, official) — audio codec integration pattern
- University of Tennessee EHS LiIon Battery Safety guidance (institutional) — battery safety practices

### Secondary (MEDIUM confidence)
- Waveshare official product/wiki pages for ESP32-S3-Touch-LCD-3.5 — board chip IDs (ST7796, FT6336, QMI8658, PCF85063, AXP2101, 16MB Flash/8MB PSRAM), accessed via search-indexed excerpts due to a blocked direct fetch
- `pioarduino/platform-espressif32` vs official platform — multiple 2024-2026 community sources (CNX Software, Circus Scientist blog, PlatformIO forum)
- `bitbank2/JPEGDEC` vs `Bodmer/TJpg_Decoder` benchmarks — atomic14.com/substack, single-source comparative benchmark
- `pschatzmann/arduino-audio-tools` ecosystem — GitHub repos + author's blog
- Supabase and Cloudflare R2 pricing — multiple 2026 third-party pricing-guide sites, cross-referenced
- Multiple hobbyist DIY project write-ups (XDA, CircuitDigest, Hackster.io, Hackaday.io, Instructables) — ESP32 retro handheld and gift-device feature patterns
- espressif/esp32-camera, espressif/esp-idf GitHub issue threads — camera/audio/SD integration gotchas

### Tertiary (LOW confidence)
- PlatformIO community forum thread on Secure Boot + Flash Encryption friction — community-reported, not an official limitation statement, needs validation
- LVGL forum / DeepWiki notes on multi-screen display management — community-sourced, not official docs
- ESP32 forum threads on brownout detection and SD/display SPI sharing — anecdotal but consistent across multiple independent reports

---
*Research completed: 2026-08-26*
*Ready for roadmap: yes*
