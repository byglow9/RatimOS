# Requirements: RatimOS

**Defined:** 2026-08-26
**Core Value:** O dispositivo tem que funcionar de verdade no dia a dia dela — offline, com as 5 seções estáveis — e continuar "vivo" depois de entregue, recebendo conteúdo novo e atualizações remotamente.

## v1 Requirements

Requirements for the gift delivery. Each maps to roadmap phases.

### Shell / Navegação

- [x] **SHELL-01**: User can navigate from home to any of the 5 sections (jogos/musica/album/cartas/config) and back
- [ ] **SHELL-02**: Home screen shows battery level and current time at all times
- [x] **SHELL-03**: Device boots directly into the home screen with a RatimOS boot identity, not a stock/blank screen

### Entrada (Touch/Stylus)

- [ ] **INPUT-01**: User can operate every screen via touch
- [ ] **INPUT-02**: User can operate every screen via stylus (pending hardware verification of passive-stylus compatibility with the capacitive touch panel — if incompatible, UI is redesigned around finger-sized targets instead)

### Energia (Bateria/RTC)

- [ ] **POWER-01**: Device shows real battery percentage/charging state sourced from the PMIC
- [ ] **POWER-02**: Device keeps correct date/time via RTC across power loss and reboot
- [ ] **POWER-03**: Device charges safely via USB-C with charge current/cutoff matched to the battery's datasheet

### Jogos

- [ ] **JOGOS-01**: User can play at least 2 complete, simple board/card games (e.g. sudoku, paciência) fully with touch/stylus input
- [ ] **JOGOS-02**: In-progress game state is not lost when navigating away and back

### Musica

- [ ] **MUSICA-01**: User can play local audio files with play/pause/next/previous/volume controls
- [ ] **MUSICA-02**: Music keeps playing in the background while browsing other RatimOS apps

### Album

- [ ] **ALBUM-01**: User can browse a thumbnail grid of photos stored on the device and view them full-screen
- [ ] **ALBUM-02**: User can capture a new photo with the onboard camera and have it appear in the album
- [ ] **ALBUM-03**: Photos synced from the cloud backend appear in the album alongside camera-captured photos

### Cartas

- [ ] **CARTAS-01**: User can browse and read a list of letters/messages stored on the device, each in a legible paginated reading view
- [ ] **CARTAS-02**: New letters synced from the cloud backend appear in the cartas list with a visible "new" indicator

### Config

- [ ] **CONFIG-01**: User can provision wifi (SSID/password) without any credential hardcoded in firmware
- [ ] **CONFIG-02**: User can re-provision/change wifi from the device if the network changes or credentials stop working
- [ ] **CONFIG-03**: User can adjust brightness and volume from the settings screen
- [ ] **CONFIG-04**: Settings screen shows basic device info (firmware version, storage used)

### Sync (Nuvem)

- [ ] **SYNC-01**: Device pulls new letters/photos/music from the cloud backend automatically when connected to wifi
- [ ] **SYNC-02**: Device shows a visible new-content indicator (badge/animation) when synced content hasn't been viewed yet
- [ ] **SYNC-03**: All 5 sections continue to work fully offline when wifi is unavailable

### OTA

- [ ] **OTA-01**: Device checks for and can download a new firmware version from the backend over HTTPS
- [ ] **OTA-02**: A failed/bad OTA update automatically rolls back to the last known-good firmware without bricking the device
- [ ] **OTA-03**: OTA capability (dual-partition scheme) is present in the very first firmware ever flashed to the device

### Segurança

- [ ] **SEC-01**: Every request between device and backend is authenticated with a unique per-device token, never a shared/global credential
- [ ] **SEC-02**: All device-backend communication uses HTTPS/TLS, never plaintext HTTP
- [ ] **SEC-03**: Device has Secure Boot and Flash Encryption enabled before being delivered as a gift
- [ ] **SEC-04**: No wifi password, API key, or token is stored in plaintext in firmware source or on removable media (SD card)

### Identidade Visual

- [ ] **VISUAL-01**: RatimOS has its own color palette, typography, and icon set, visually distinct from the "colombiaOS" reference project

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Polish

- **POLISH-01**: Additional games (more board/card variety)
- **POLISH-02**: Richer music features (playlists, shuffle)
- **POLISH-03**: Idle/screensaver mode showing rotating photos or clock
- **POLISH-04**: Sleep/power-management tuning for longer battery life, based on real-world usage patterns

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Console emulation (NES/GBA/SMS) | Bottomless complexity/compatibility pit; board/card games deliver the "jogos" requirement with far less risk |
| Custom PCB design | Stacks PCB layout/SMD soldering on top of a first-ever embedded project for a solo, no-deadline developer; commercial board is sufficient |
| Companion mobile app | No stated need; device is the only interface she uses — a simple developer-side admin/upload tool is enough |
| On-device content authoring (writing letters, editing photos, camera filters) | Conflicts with the one-directional gift model — she receives content, developer sends it |
| Real-time chat / instant push notifications | Adds websocket/push infrastructure and battery cost for content that's inherently asynchronous (letters/photos aren't chat messages) |
| Voice assistant / AI chatbot personality | Shifts emotional register from "handmade personal gift" to "generic AI gadget"; not requested |
| Multi-user accounts / cloud dashboard with logins | Exactly one recipient and one operator — auth complexity is pure waste |
| Elaborate theme engine / boot animation skinning | Turns a one-time styling pass into an open-ended configurability project that never converges |
| Fixed delivery deadline | No date pressure — phases proceed sequentially until the device is stable and ready |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| SHELL-01 | Phase 1 | Complete |
| SHELL-02 | Phase 4 | Pending |
| SHELL-03 | Phase 1 | Complete |
| INPUT-01 | Phase 3 | Pending |
| INPUT-02 | Phase 3 | Pending |
| POWER-01 | Phase 4 | Pending |
| POWER-02 | Phase 4 | Pending |
| POWER-03 | Phase 4 | Pending |
| JOGOS-01 | Phase 9 | Pending |
| JOGOS-02 | Phase 9 | Pending |
| MUSICA-01 | Phase 5 | Pending |
| MUSICA-02 | Phase 5 | Pending |
| ALBUM-01 | Phase 5 | Pending |
| ALBUM-02 | Phase 5 | Pending |
| ALBUM-03 | Phase 7 | Pending |
| CARTAS-01 | Phase 5 | Pending |
| CARTAS-02 | Phase 7 | Pending |
| CONFIG-01 | Phase 6 | Pending |
| CONFIG-02 | Phase 6 | Pending |
| CONFIG-03 | Phase 6 | Pending |
| CONFIG-04 | Phase 6 | Pending |
| SYNC-01 | Phase 7 | Pending |
| SYNC-02 | Phase 7 | Pending |
| SYNC-03 | Phase 7 | Pending |
| OTA-01 | Phase 8 | Pending |
| OTA-02 | Phase 8 | Pending |
| OTA-03 | Phase 3 | Pending |
| SEC-01 | Phase 2 | Pending |
| SEC-02 | Phase 2 | Pending |
| SEC-03 | Phase 10 | Pending |
| SEC-04 | Phase 10 | Pending |
| VISUAL-01 | Phase 9 | Pending |

**Coverage:**

- v1 requirements: 32 total (corrected from earlier "27" placeholder — 27 was an early estimate before all categories were finalized; the 32 REQ-IDs actually listed above are the authoritative set)
- Mapped to phases: 32/32 ✓
- Unmapped: 0 ✓

---
*Requirements defined: 2026-08-26*
*Last updated: 2026-08-26 after roadmap creation (traceability filled in, requirement count corrected to 32)*
