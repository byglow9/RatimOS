<!-- GSD:project-start source:PROJECT.md -->

## Project

**RatimOS**

RatimOS é um sistema operacional e dispositivo tipo "celular retrô" construído do zero como presente único e pessoal para a namorada do desenvolvedor. Roda em uma placa ESP32-S3 com tela touch, com apps de jogos, música, álbum de fotos, cartas e configurações, bateria própria, relógio, câmera e uma estética visual autoral (inspirada, mas não copiada, de um projeto de referência de terceiros conhecido como "colombiaOS").

**Core Value:** O dispositivo tem que funcionar de verdade no dia a dia dela — offline, com as 5 seções (jogos, música, álbum, cartas, config) estáveis — e continuar "vivo" depois de entregue, recebendo cartas/fotos/músicas novas e atualizações de firmware remotamente, sem o desenvolvedor precisar pegar o aparelho de volta.

### Constraints

- **Tech stack**: ESP32-S3 + PlatformIO + Arduino core + LVGL — escolhido por ser o caminho mais amigável pra quem nunca fez desenvolvimento embarcado, com possibilidade de migrar partes pra ESP-IDF puro depois se precisar de mais controle.
- **Orçamento**: baixo custo — placa principal ~R$330-360, mais bateria LiPo, cartão microSD e módulo de câmera OV5640 se não vier incluso.
- **Timeline**: sem prazo fixo — o presente é entregue quando o hardware estiver pronto e estável, não numa data específica.
- **Hardware disponível**: nenhum ainda — todo desenvolvimento inicial precisa ser testável em simulador de PC (SDL2) antes da placa chegar.
- **Segurança**: dispositivo vai ficar conectado à rede wifi pessoal dela — não pode virar porta de entrada pra rede doméstica nem expor dados íntimos (fotos, cartas) sem autenticação adequada.

<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->

## Technology Stack

## Recommended Stack

### Core Firmware Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Arduino core for ESP32 (`espressif/arduino-esp32`) | 3.3.x (based on ESP-IDF 5.5) | Firmware framework | Already the project's chosen toolchain. As of 2026 the 3.x line is the current stable branch; it's a thin layer over ESP-IDF 5.5, so IDF components (esp32-camera, esp_https_ota, mbedtls) remain usable directly. Confidence: MEDIUM. |
| PlatformIO `platform-espressif32` | Use the **`pioarduino` community fork**, not the original `platformio/platform-espressif32` | Board/toolchain definitions for PlatformIO | The official PlatformIO-maintained platform has historically lagged behind Espressif's Arduino core releases (new chip variants, latest IDF). The `pioarduino` fork tracks upstream `arduino-esp32` releases much faster and is the de-facto community standard for ESP32-S3 projects on PlatformIO in 2025-2026. Set `platform = https://github.com/pioarduino/platform-espressif32/releases/download/<tag>/platform-espressif32.zip` in `platformio.ini`. Confidence: MEDIUM (community consensus, actively confirmed by multiple 2025-2026 sources; small maintainer team is a mild sustainability risk — pin the release tag). |
| LVGL | 9.5.x (current v9 line) | UI toolkit | Already chosen. v9.5 is the current stable release of the v9 major; the LVGL PC/SDL2 simulator config the project already uses maps 1:1 onto the same `lv_conf.h`/driver abstraction used on real hardware, so no rewrite is needed when hardware arrives — only the display/touch driver layer changes. Confidence: HIGH (official LVGL changelog). |
| LovyanGFX **or** `esp_lcd` (IDF driver, wrapped for Arduino) as the display/touch backend for LVGL | LovyanGFX latest (Arduino Library Manager) | Panel + touch driver feeding LVGL's flush/read callbacks | The Waveshare 3.5" board uses an **ST7796** SPI display controller and an **FT6336** I2C capacitive touch controller (confirmed via Waveshare's own product/wiki pages). Both are supported out of the box by LovyanGFX, which is the most common Arduino-side driver for LVGL v9 on ESP32 boards and handles DMA-based SPI flush, rotation, and touch calibration with far less boilerplate than hand-rolling `esp_lcd` panel APIs. Recommend starting with LovyanGFX; fall back to raw `esp_lcd` (ESP-IDF) only if you hit a LovyanGFX limitation. Confidence: MEDIUM (board chip IDs confirmed from vendor docs; LovyanGFX ST7796/FT6336 compatibility confirmed from community sources, not yet tested on this exact board). |

### Audio (ES8311 codec — music player)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `pschatzmann/arduino-audio-tools` | latest (Arduino Library Manager / GitHub) | I2S audio pipeline, codec glue, streaming | The most actively maintained, widely used Arduino-ecosystem audio framework for ESP32. It has first-class ES8311 support (via `arduino-audio-driver`) and a clean Stream-based API for feeding I2S output from SD-card files or decoders. It's the closest thing to a "standard" for ESP32 + I2S codec chips in Arduino land. Confidence: MEDIUM. |
| `pschatzmann/arduino-audio-driver` | latest | Codec (ES8311) init/config over I2C + I2S clocking | Purpose-built codec driver library that pairs with AudioTools; explicitly lists ES8311 as supported. Do NOT use Espressif's own standalone `es8311` IDF component directly for new code — Espressif has marked it **deprecated in favor of `esp_codec_dev`**; the pschatzmann driver wraps the equivalent logic with an Arduino-friendly API and is easier to integrate outside full ESP-ADF. Confidence: MEDIUM. |
| `pschatzmann/arduino-libhelix` | latest | MP3 (and AAC) decode | Helix-based decoder purpose-built to plug into AudioTools' pipeline; this is the standard, low-footprint MP3 decode path for ESP32 in the Arduino ecosystem (not full ESP-ADF). Use MP3 as your music library's canonical format — smaller SD-card footprint than WAV, decode cost is trivial for ESP32-S3 at 240 MHz. Confidence: MEDIUM. |

### Photos (JPEG decode — album app + camera capture)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `bitbank2/JPEGDEC` | latest (PlatformIO registry: `bitbank2/JPEGDEC`) | JPEG decode → LVGL canvas/image | Benchmarked as the fastest of the common Arduino-ecosystem JPEG decoders for embedded targets: it decodes in 128×16 pixel blocks (vs. 16×16 for TJpg_Decoder), which reduces per-block callback overhead and allows better DMA overlap. Recommended default for the photo album, especially since photos will come from the camera (OV5640, up to 5MP) and from synced files — decode speed matters for a smooth gallery UX on a 320×480 screen. Confidence: MEDIUM (comparative benchmarks from a single well-regarded source, not independently reproduced by this research). |
| `Bodmer/TJpg_Decoder` | latest | JPEG decode (simpler alternative) | Fallback / simpler option if JPEGDEC's API proves awkward to integrate with LVGL's image drawing. It's TJpgDec-based (same decompressor ROM'd on ESP32-S3), well documented, huge install base, but decodes in smaller 16×16 blocks (slower for large images). Fine for thumbnail-sized rendering. Confidence: MEDIUM. |
| `espressif/esp_jpeg` (ESP Component Registry) | v1.3.x | Alternative: IDF-native decode/encode, useful specifically for **camera capture → JPEG → downscale/thumbnail** pipeline | If integrating via ESP-IDF component manager (works fine inside a PlatformIO+Arduino project through `idf_component.yml`), this is Espressif's own lightweight TJpgDec-based component — smaller flash footprint than pulling in a full Arduino wrapper, and pairs naturally with `esp32-camera`'s JPEG output. Consider this specifically for the **encode-on-capture** path (compressing a fresh OV5640 photo before writing to SD/syncing), while using JPEGDEC/TJpg_Decoder for **decode-on-display** in the LVGL gallery. Confidence: MEDIUM. |

### Camera (OV5640 capture pipeline)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `espressif/esp32-camera` | v2.1.x (ESP Component Registry) | DVP camera capture driver | This is THE standard camera driver for ESP32/S2/S3 in both Arduino and IDF projects — it's what every ESP32-CAM/OV5640 tutorial and product (including Waveshare's own camera-equipped boards) builds on. It has explicit OV5640 sensor support plus an **autofocus helper specifically for OV5640** (enable via menuconfig / component config), and outputs JPEG directly from the sensor pipeline (hardware/software JPEG depending on chip), which slots straight into the `esp_jpeg`/SD-write path above. Confirm the Waveshare board's camera header pinout against `esp32-camera`'s pin config struct before wiring — DVP interfaces vary board to board. Confidence: MEDIUM (library-sensor support confirmed from official repo issues/docs; exact pin mapping for this specific Waveshare board not yet independently verified — flag for phase-specific research once hardware/schematic in hand). |

### Storage (SD card + filesystem)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `SD_MMC` (bundled with arduino-esp32 core) | bundled | microSD access for media/messages (photos, music, letters, local save data) | Use the **SDMMC (4-bit) interface, not SD-over-SPI**, if the Waveshare board wires the microSD slot to dedicated SDMMC pins (verify against the board's pinout doc — ESP32-S3 has a dedicated SDMMC host peripheral). SDMMC in 4-bit mode is roughly 2x the throughput of SD-over-SPI, which matters for smooth JPEG/MP3 streaming off the card. Both expose a standard FAT filesystem via Arduino's `FS.h`, so app code doesn't need to know which transport is in use. Confidence: MEDIUM — pin availability on this exact board needs confirmation once schematic/pinout is read (fall back to `SD.h` SPI mode if SDMMC pins are unavailable/shared with the display bus). |
| FAT32 (formatted on the microSD card itself, not LittleFS) | — | Filesystem for removable media | Keep the microSD card FAT32-formatted so it stays plug-and-play if it ever needs to be read on a PC for debugging/manual content loading. Use the internal SPI NOR flash (16 MB on this board) for firmware + a small **LittleFS or NVS partition** for device config/state (WiFi provisioning result, device token, last-sync timestamps) — never store the device's auth token or WiFi credentials on the removable SD card. |
| Preferences / NVS (`Preferences.h`, bundled) | bundled | Small persistent key-value state (device token, last sync cursor, provisioning flag) | Standard Arduino-ESP32 wrapper over IDF's NVS partition — appropriate for small structured state that must survive reboots/OTA. |

### Connectivity — WiFi Provisioning (BLE, no hardcoded credentials)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| ESP-IDF `wifi_provisioning` component (via `WiFiProv`/`WiFiProvisioner` Arduino wrapper, or IDF component directly) | bundled with arduino-esp32 3.x / ESP-IDF 5.5 | BLE-based WiFi credential provisioning at first boot / after reset | This is Espressif's own standard provisioning stack (`protocomm` + `wifi_provisioning`), used by their own "ESP BLE Provisioning" companion app (Android/iOS, actively updated in 2026). It transfers WiFi credentials over BLE with Proof-of-Possession security, meaning your girlfriend (or you, remotely, never again) can (re-)provision WiFi from a phone without ever putting a password in the firmware source. This directly satisfies the project's "no hardcoded credentials" requirement. Confidence: MEDIUM (official Espressif component, mechanism confirmed; wiring it up cleanly from Arduino rather than raw IDF needs validation once implementation starts). |

### Connectivity — OTA Firmware Updates

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `HTTPUpdate` (bundled `Update.h` + `HTTPUpdate.h` in arduino-esp32 core) over HTTPS | bundled | Pull-based OTA: device checks a URL, downloads new firmware, flashes inactive OTA partition | Bundled with the core (no extra library needed), supports HTTPS via `NetworkClientSecure`, and follows the standard ESP32 dual-partition (app0/app1) safety model — a failed/interrupted OTA never bricks the device because the bootloader only switches partitions after a verified-good write. This is the right model for "device checks in periodically and self-updates without physical access," which is exactly this project's requirement. Confidence: MEDIUM (mechanism and safety model confirmed across official Espressif examples/docs and multiple tutorials). |
| Custom partition table (`partitions.csv`) sized for **ota_0 / ota_1** dual app partitions + a data partition | — | OTA-safe flash layout | With 16 MB flash on this board, a standard dual-OTA layout (e.g., ~6 MB per app slot) comfortably fits LVGL + app code + audio/JPEG libraries with room to grow, while still leaving space for a SPIFFS/LittleFS config partition. Must be defined explicitly in PlatformIO (`board_build.partitions`); do not rely on the default single-app partition scheme. |
| Version check before download (compare running FW version string against server-advertised version, e.g. via a small JSON manifest) | — | Avoid unnecessary re-flash / bandwidth waste | Standard practice — pair with the backend's manifest endpoint (see below) so the device only downloads when a newer build is actually available. |

### Backend (cloud sync + OTA hosting) — solo hobbyist, near-zero budget

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **Supabase** (Free tier) | current SaaS | Postgres DB (metadata: letters, sync cursors, device registry) + Auth (device tokens) + Storage (photos/music/firmware binaries, S3-compatible) + Edge Functions (manifest/upload endpoints) | Best fit for "solo hobbyist, zero backend experience, low/no budget": one dashboard covers DB + object storage + auth + serverless functions, generous free tier (500 MB DB, 1 GB file storage, 5 GB egress/month, 500k edge function invocations), and a REST/JS-friendly API you can also call from a simple device-side HTTPS request without needing to hand-roll a server. **Caveat:** free-tier projects auto-pause after 7 days of *zero* activity — but since the device itself will poll for sync/OTA on a schedule, that polling keeps the project warm as long as the interval is well under a week (e.g., daily/weekly check-ins). Confidence: MEDIUM (pricing/limits cross-verified across multiple 2026 sources; the auto-pause interaction with device polling frequency is a reasoned inference, not independently tested). |
| **Cloudflare R2** (Free tier) as an alternative/companion object store for large media + firmware binaries | current SaaS | Zero-egress binary storage for OTA `.bin` files, full-resolution photos, music files | If Supabase's 1 GB storage or 5 GB egress ceiling gets tight (likely once photo/music libraries grow), R2's 10 GB free storage with **zero egress fees** (unlike Supabase Storage, which is S3-compatible but still counts against egress) makes it the cheaper long-term home for large binary blobs, while Supabase Postgres stays the source of truth for metadata (letters text, timestamps, "what's new" flags) and Auth. This is a common hybrid pattern: Postgres+Auth in Supabase, bulk objects in R2, referenced by URL. Confidence: MEDIUM. |
| Plain HTTPS + a device-specific bearer token (no MQTT broker, no push) | — | Sync transport pattern: device pulls, never receives pushes | Given the device has no guaranteed uptime/connectivity and no need for real-time delivery (letters/photos/OTA are all "eventually, next time it's on WiFi" use cases), a simple periodic HTTPS GET against a manifest endpoint (return list of new content + latest firmware version) is dramatically simpler to build, secure, and debug for a solo hobbyist than standing up MQTT/WebSocket infrastructure. Each device gets one long-lived bearer token (stored in NVS, never on SD card, never in source), sent as an `Authorization` header on every request — satisfies the project's "communication always via HTTPS with a per-device token" security constraint directly. |

### Security (device-side)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| ESP32-S3 Secure Boot v2 + Flash Encryption | ESP-IDF 5.5 mechanism | Prevent firmware tampering / flash readout after physical delivery | Both are IDF-level features (eFuse-backed) and are explicitly listed as a project requirement ("Secure Boot + Flash Encryption" before delivery). **Important caveat found in research:** enabling these fully (custom signing keys, Release-mode flash encryption) is significantly harder from pure PlatformIO+Arduino tooling than from raw ESP-IDF — community reports describe friction getting Secure Boot v2 + Flash Encryption working cleanly through PlatformIO's Arduino framework. Plan for this as its own late-stage phase, budget time to either (a) invoke `idf.py`-level tooling from within the PlatformIO project (PlatformIO does expose the underlying ESP-IDF build for `arduino` framework projects on IDF-based cores) or (b) do the signing/encryption enablement as a one-time manual step using `espsecure.py` against the compiled binaries before final flashing. **This is undoubtedly a one-way trip in Release mode** — losing the signing key means the device can never receive new firmware again, so back up the key pair before ever enabling Release-mode flash encryption. Confidence: MEDIUM (mechanism and difficulty both cross-verified across official IDF docs and PlatformIO community threads; exact Arduino-framework workflow not yet validated for this project). |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `bblanchon/ArduinoJson` | 7.4.x | Parse/serialize JSON (sync manifests, config, backend API responses) | Universal — use for every device↔backend JSON exchange. v7's API differs from v6 (uses `JsonDocument` without a fixed capacity template); write new code against v7 directly rather than following older v6 tutorials. |
| `lewisxhe/XPowersLib` | latest | AXP2101 PMIC driver — battery charge management, power rail control, charge status/percentage reporting | Required for battery UI (charge %, low-battery warnings) and for correctly sequencing power rails at boot. Actively maintained, explicitly verified against ESP32-S3, MIT licensed. Use for anything touching the AXP2101 (do not bit-bang the I2C registers by hand). |
| RTC PCF85063 library — evaluate `SolderedElectronics/PCF85063A-Arduino-Library` first | latest | RTC read/write over I2C for the onboard PCF85063 (clock UI, timestamping photos/letters, "last synced" display) | The PCF85063 (not the very similar but distinct PCF8563) has fewer dedicated Arduino libraries; the Soldered library targets the exact PCF85063A part. If it proves awkward, a thin custom I2C wrapper (the register map is small and well documented) is a reasonable fallback — don't over-invest here. |
| QMI8658 IMU driver (search Arduino Library Manager / GitHub for `QMI8658`) | latest | Optional: motion/orientation for UI flourishes (auto-rotate, shake-to-shuffle music, step counter for a fun stat) | Not a core requirement per PROJECT.md, but the chip is already on the board at zero marginal cost — worth a small "Easter egg" differentiator feature once the 5 core sections are stable. Low priority. |
| PlatformIO **native** environment (`platform = native`) for the SDL2 simulator target | — | Already in use — keep the LVGL app/UI logic in a hardware-agnostic layer so the same code paths run in both the `native` (SDL2, PC) and `esp32-s3` (real hardware) PlatformIO environments, swapping only the display/touch/storage driver implementations. | Continue this pattern as new subsystems (audio, camera, sync) are added — stub/mock the hardware-only pieces (audio codec, camera, BLE) behind an interface so UI and app logic remain testable on PC before each new physical subsystem arrives. |

## Installation

# idf_component.yml — for IDF-registry components used inside the Arduino framework project

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| PlatformIO ESP32 platform | `pioarduino` fork | Official `platformio/platform-espressif32` | Slower to track new arduino-esp32/IDF releases; picking the fork now avoids a painful mid-project platform migration later. |
| Display/touch driver | LovyanGFX | Raw `esp_lcd` IDF panel API | More boilerplate, less LVGL-integration tooling; only worth it if LovyanGFX hits a hard limitation on this exact panel. |
| JPEG decode (display) | JPEGDEC | TJpg_Decoder / esp_jpeg | TJpg_Decoder is simpler but slower (16×16 blocks); esp_jpeg is IDF-native and great for the *encode* side, less ergonomic for Arduino-side *decode-to-LVGL-canvas* wiring. |
| Audio pipeline | AudioTools + arduino-audio-driver + arduino-libhelix | Full ESP-ADF (Espressif Audio Development Framework) | ESP-ADF is powerful but heavyweight, has its own build-system expectations that fight PlatformIO+Arduino, and is overkill for "play MP3s from SD through one codec." AudioTools is the pragmatic Arduino-ecosystem equivalent. |
| Backend | Supabase (+ optional Cloudflare R2 for bulk objects) | Firebase | Firebase's free tier and pricing model are comparably usable, but Supabase's Postgres-based data model is more transparent/debuggable for a beginner (plain SQL you can inspect), and bundles Storage+Auth+Edge Functions in the same free project without vendor lock-in to a proprietary NoSQL query language. |
| Backend | Supabase (+ optional Cloudflare R2) | Self-hosted (PocketBase, Node/Express + SQLite, etc.) on a VPS | PROJECT.md explicitly rules out relying on the developer's own always-on machine; a self-hosted VPS reintroduces "must keep infrastructure alive myself" risk and ops burden that a managed free-tier SaaS avoids — the whole point of this dimension is minimizing backend maintenance for a solo hobbyist. |
| OTA transport | HTTPUpdate (pull, HTTPS) | ArduinoOTA (LAN-only, `ArduinoOTA.handle()` in loop) / ElegantOTA (local web UI) | Both alternatives assume the developer is on the same local network as the device, which directly contradicts the "no further physical/local-network access after delivery" requirement. Pull-over-HTTPS from a public backend is the only viable model here. |
| WiFi provisioning | ESP-IDF BLE provisioning (`wifi_provisioning` + companion app) | SoftAP captive portal (`WiFiManager` library) | SoftAP provisioning works but requires the end user to join a temporary "RatimOS-Setup" WiFi network and open a browser — clunkier for a non-technical gift recipient than a guided phone-app BLE flow. BLE provisioning is the more polished, "it just works" option and is what Espressif's own official app targets. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Hand-rolling ES8311 register writes via raw I2C, or using Espressif's now-deprecated standalone `es8311` IDF component | Espressif itself points users toward `esp_codec_dev` as the successor; the raw component is unmaintained for new projects | `pschatzmann/arduino-audio-driver` (Arduino-ergonomic) or `esp_codec_dev` (if going full IDF-component route) |
| MQTT/WebSocket broker for sync | Massive overkill for "pull new letters/photos/music every so often" — adds a whole class of always-on infrastructure (a broker) that a solo hobbyist would then have to run and secure | Simple periodic HTTPS GET against a JSON manifest endpoint (see backend section) |
| Storing WiFi password or device auth token as a literal string in firmware source / committed to git | Directly violates the project's own security constraint; also means every future device needs a unique firmware build | BLE provisioning for WiFi; NVS-stored, backend-issued per-device bearer token for auth (provisioned once at first boot via a short-lived pairing code, not hardcoded) |
| SD-over-SPI when SDMMC pins are available and unshared | Roughly half the throughput of SDMMC 4-bit mode — matters once you're streaming MP3/JPEG off the card during UI interaction | `SD_MMC` (SDMMC host, 4-bit) — confirm pin availability against this board's schematic first |
| ESP-ADF (Espressif Audio Development Framework) as the primary audio stack | Heavyweight, opinionated build system, steep learning curve for someone new to embedded — mismatched with the project's explicit "gentlest on-ramp" tooling philosophy (PlatformIO + Arduino) | `pschatzmann/arduino-audio-tools` ecosystem |
| Firebase Realtime Database / Firestore as the primary backend | Proprietary query model, historically has had free-tier "spark plan" changes that surprise hobbyists, and doesn't naturally bundle S3-compatible object storage the way Supabase does | Supabase (Postgres + Storage + Auth + Edge Functions in one free project) |
| Default single-app PlatformIO partition scheme | Leaves no room for a second OTA app slot, defeating the whole safe-OTA model | Custom `partitions.csv` with explicit `ota_0`/`ota_1` + `nvs` + data partitions, sized against this board's 16 MB flash |

## Stack Patterns by Variant

- Move large binary objects (full-resolution photos, music files, `.bin` firmware images) to Cloudflare R2 (10 GB free, zero egress)
- Keep Supabase for Postgres metadata (what's new, timestamps, device registry) + Auth
- Device-side change is minimal: manifest endpoint just returns R2 URLs instead of Supabase Storage URLs for large objects
- Fall back to ESP-IDF's native `esp_lcd_panel_st7796` + a manual I2C touch read for FT6336, wrapped in LVGL's `lv_display_t`/`lv_indev_t` driver structs directly — more code, but a documented, well-trodden path for LVGL v9 on ESP-IDF
- Treat "finalize and lock down the device" as a dedicated late-phase task where you invoke the underlying ESP-IDF toolchain (`idf.py` / `espsecure.py`) directly against the PlatformIO-built binaries, rather than trying to force the entire security workflow through `pio run` — this is a one-time operation per production unit, not something that needs to be scripted into the everyday PlatformIO dev loop

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| `arduino-esp32` 3.3.x | ESP-IDF 5.5 (underlying) | Determines which IDF-level APIs (mbedtls config, esp_https_ota, wifi_provisioning) are available under the hood. |
| `pioarduino/platform-espressif32` release tag | a specific `arduino-esp32` core version | Pin an explicit release tag in `platformio.ini` (not a moving branch reference) so builds are reproducible across the project's long, no-deadline timeline. |
| LVGL 9.5.x | `lv_conf.h` schema from LVGL v9 (not v8) | The project's existing SDL2 simulator config should already be on the v9 schema — verify `lv_conf.h` hasn't drifted before adding hardware drivers. |
| ArduinoJson v7 | API differs from v6 | Any tutorial/snippet referencing `StaticJsonDocument<N>` is v6-era; v7 uses a single `JsonDocument` type with dynamic-ish internal allocation — don't mix v6 patterns into v7 code. |
| `esp32-camera` v2.1.x | ESP32-S3 DVP camera interface | Only supports DVP-interface sensors (OV5640 included) — confirm the Waveshare board's camera header is DVP, not MIPI-CSI, before wiring (S3's camera peripheral in this class of board is DVP; MIPI-CSI is an ESP32-P4 feature, not S3). |

## Sources

- Waveshare official product/wiki pages for ESP32-S3-Touch-LCD-3.5 (ST7796 display driver, FT6336 touch, QMI8658 IMU, PCF85063 RTC via AXP2101, 16 MB Flash / 8 MB PSRAM, DVP camera interface) — confidence MEDIUM (vendor-published, cross-referenced across multiple Waveshare pages, WebFetch of the primary wiki page itself was blocked by a 403 so this relies on search-indexed excerpts)
- `espressif/arduino-esp32` GitHub releases (core version 3.3.11 / ESP-IDF 5.5, 2026-07-22) — confidence MEDIUM
- `pioarduino/platform-espressif32` vs official `platformio/platform-espressif32` — multiple 2024-2026 community sources (CNX Software, Circus Scientist blog, PlatformIO community forum) — confidence MEDIUM
- LVGL official changelog (lvgl.io/docs) — v9.5.0 current — confidence HIGH
- `espressif/esp32-camera` GitHub repo + ESP Component Registry (OV5640 + autofocus helper support) — confidence MEDIUM
- `espressif/esp_jpeg` ESP Component Registry + Espressif developer blog (esp_new_jpeg introduction) — confidence MEDIUM
- `bitbank2/JPEGDEC` vs `Bodmer/TJpg_Decoder` block-size/performance comparison — atomic14.com/atomic14.substack.com benchmarks — confidence MEDIUM (single-source benchmark, not independently reproduced)
- `pschatzmann/arduino-audio-tools`, `arduino-audio-driver`, `arduino-libhelix` GitHub repos and author's blog (pschatzmann.ch) — confidence MEDIUM
- Espressif ESP-IDF docs: SDMMC Host Driver, Mbed TLS RAM usage, Secure Boot v2, Flash Encryption (docs.espressif.com) — confidence MEDIUM-HIGH (official docs)
- PlatformIO community forum thread on Secure Boot + Flash Encryption friction under PlatformIO/Arduino framework — confidence LOW-MEDIUM (community-reported difficulty, not an official limitation statement)
- `lewisxhe/XPowersLib` GitHub repo (AXP2101 support, verified on ESP32-S3) — confidence MEDIUM
- `bblanchon/ArduinoJson` official site/GitHub releases (v7.4.3, 2026-07-06) — confidence MEDIUM
- Supabase pricing/free-tier comparisons (multiple 2026 pricing-guide sites, cross-referenced for consistency: 500 MB DB / 1 GB storage / 5 GB egress / 7-day pause on inactivity) — confidence MEDIUM (third-party pricing summaries, not fetched directly from supabase.com due to no direct fetch performed this session — recommend re-verifying exact current numbers against supabase.com/pricing before committing budget assumptions)
- Cloudflare R2 pricing (developers.cloudflare.com/workers/platform/pricing + multiple 2026 third-party pricing breakdowns, cross-referenced: 10 GB free storage, zero egress) — confidence MEDIUM
- ESP-IDF Wi-Fi Provisioning docs + ESP BLE Provisioning companion app listing (Google Play, updated May 2026) — confidence MEDIUM
- arduino-esp32 `HTTPUpdate`/`Update` library examples and OTA partition-safety explanations (official repo + DeepWiki + multiple tutorials, cross-referenced) — confidence MEDIUM

<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->

## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->

## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->

## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->

## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:

- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->

## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
