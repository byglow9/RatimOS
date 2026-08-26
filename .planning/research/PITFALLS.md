# Pitfalls Research

**Domain:** ESP32-S3 + LVGL handheld personal device (touchscreen, battery, camera, SD card, audio, wifi sync, OTA) — built by a first-time embedded developer, to be gifted and remotely maintained
**Researched:** 2026-08-26
**Confidence:** MEDIUM-HIGH (Espressif official docs + ESP-IDF issue tracker + LVGL docs/forum + multiple independent hobbyist reports cross-checked; no project-specific hands-on validation possible pre-hardware)

## Critical Pitfalls

### Pitfall 1: Capacitive touch panel doesn't reliably sense a passive stylus

**What goes wrong:**
The Waveshare ESP32-S3-Touch-LCD-3.5 uses a capacitive touch controller (GT-series, I2C). Capacitive touch works by sensing the electrical capacitance of a finger; a passive (non-conductive, non-electronic) stylus tip does not couple capacitively the same way and is either not detected at all or detected inconsistently. This directly conflicts with the project requirement "tela touch funciona com caneta/stylus, não só com o dedo."

**Why it happens:**
Hobbyists conflate "touchscreen" with "stylus support" — resistive touchscreens (pressure-based) work with any stylus, capacitive ones generally don't unless it's an active/capacitive-tip stylus or the firmware specifically compensates. This board was chosen for QSPI GPIO budget reasons, not for touch technology, so stylus compatibility was never actually verified.

**How to avoid:**
- Verify with the actual touch controller datasheet (I2C address, driver IC) whether it supports passive-stylus detection, before assuming.
- Test with a capacitive-tip stylus (conductive rubber/mesh tip) — these work fine on standard capacitive panels and are cheap (~R$10-20).
- If a hard, precise "pen" feel is required (e.g. for a "cartas" handwriting app), budget for either (a) buying a stylus explicitly rated for capacitive touch, or (b) accepting reduced precision and designing the "writing" UI around finger-sized targets instead of pen-like precision.
- Do NOT assume a random plastic stylus will work — test on hardware as soon as it arrives, before building any UI that depends on stylus precision.

**Warning signs:**
Stylus touches register inconsistently, only work when pressed very hard, or don't register at all while finger touches work fine.

**Phase to address:**
Early hardware bring-up phase (first phase after hardware arrives) — must be verified before building any handwriting/drawing UI ("cartas" section), since this determines whether that feature's UX needs to be redesigned around finger input instead.

---

### Pitfall 2: Shared QSPI/PSRAM bandwidth causes camera, display, and SD card to starve each other

**What goes wrong:**
On ESP32-S3, PSRAM (used for LVGL frame buffers), the QSPI display, the camera (DVP/JPEG frames often DMA'd into PSRAM), and an SD card can all compete for the same memory bus and/or SPI peripheral bandwidth. Symptoms: LVGL animations stutter/flicker when the camera is active, camera frames drop or corrupt when Wi-Fi is transmitting, or SD card reads/writes cause visible display hitches. Writing camera YUV/RGB frames to PSRAM is inherently slow and gets worse when Wi-Fi is also active (both compete for the same internal bus arbitration).

**Why it happens:**
Beginners treat "the board has a camera interface and QSPI display" as meaning these subsystems are independent, when in fact they share a single PSRAM chip and the ESP32-S3's internal memory bus is a scarce shared resource. Default board-support-package configs often don't enable PSRAM DMA for the camera or don't run PSRAM at its maximum supported clock, silently leaving performance on the table until multiple subsystems run simultaneously.

**How to avoid:**
- Confirm PSRAM is configured for maximum supported speed/mode (Octal vs Quad SPI PSRAM affects this significantly) in `sdkconfig`/PlatformIO board config — check what mode this specific PSRAM chip supports.
- Enable `CONFIG_CAMERA_PSRAM_DMA` (or equivalent Arduino-core camera config) explicitly; it defaults to off.
- Never assume "camera live-preview + LVGL animation + Wi-Fi sync" running simultaneously will perform the same as each running alone — budget a dedicated integration test phase for this specific combination.
- Prefer capturing camera stills (not continuous video/live-preview) since RatimOS only needs photo capture for the album, not video — this sidesteps most of the sustained-bandwidth contention.
- Use JPEG capture mode (hardware-compressed by the camera) rather than raw RGB/YUV to reduce PSRAM write volume.

**Warning signs:**
Display stutters/flickers only when camera or SD card is simultaneously active; camera photos come out corrupted/torn specifically when Wi-Fi sync is running in the background.

**Phase to address:**
Hardware bring-up phase (camera integration) and again at the phase where camera capture + wifi sync + display run concurrently — treat "all subsystems active together" as its own explicit integration milestone, not an assumed byproduct of individual feature phases.

---

### Pitfall 3: LiPo battery management done wrong is a fire/safety risk, not just a bug

**What goes wrong:**
The board has a PMIC (AXP2101) that should handle charge/discharge safely — but misconfiguring it (wrong charge current, wrong cell voltage cutoffs, wrong battery chemistry profile) or bypassing it (e.g. wiring a raw LiPo directly to unregulated points, using the wrong connector polarity, mechanically damaging the pouch cell) can cause overcharging, overheating, swelling, or in the worst case fire. This is qualitatively different from a software bug — a mistake here can literally injure the person the device is being gifted to, or damage their home.
DIY software fuel-gauge implementations (voltage-based SoC estimation) are also unreliable — unlike a fuel-gauge IC (e.g. built into the AXP2101, or an external MAX1704x), simple ADC-voltage-to-percentage mapping drifts and doesn't account for load/temperature, giving misleading battery percentages.

**Why it happens:**
First-time embedded developers treat "battery" as a software/firmware concern only, underestimating that Li-ion/LiPo cells are the single most safety-critical component in the whole build. They also may not realize the PMIC has a specific correct configuration (charge current limit matched to the actual battery's mAh rating, correct cutoff voltages) that must be set deliberately, not left at power-on defaults which may assume a different battery capacity.

**How to avoid:**
- Use the PMIC's (AXP2101) built-in charge management and fuel gauge — do not build a custom charging circuit or a hand-rolled voltage-divider SoC estimator as the primary source of truth.
- Match the charge-current register setting to the actual LiPo cell's rated capacity (common rule of thumb: charge current ≤ 1C, often set lower like 0.5C for longevity) — read the specific cell's datasheet, don't guess.
- Buy a LiPo cell with a built-in protection circuit (over-charge/over-discharge/over-current protection PCB) — nearly all reputable hobbyist LiPo packs include this; verify before buying a bare cell.
- Never leave a charging LiPo unattended for the first several charge cycles; charge on a fireproof/non-flammable surface (ceramic plate, LiPo bag) during initial bring-up.
- Physically inspect the cell before every use for puncture, swelling, or damage — never use a puffy/swollen cell.
- Get the polarity of the battery connector right before first power-on — reversed polarity on a raw JST connector is a common beginner wiring mistake that can destroy the PMIC or the cell.

**Warning signs:**
Battery becomes noticeably warm/hot during charge (should stay near room temperature), any swelling/puffiness, odd smell, or charge percentage that jumps erratically rather than trending smoothly.

**Phase to address:**
Power management phase (battery + PMIC integration), before the device is ever left unattended charging — this should be one of the first hardware phases, done deliberately and cautiously rather than rushed, since it's the phase with actual physical safety risk (not just "the app crashes").

---

### Pitfall 4: OTA update without confirmed rollback bricks a device the developer can no longer physically reach

**What goes wrong:**
ESP32's OTA mechanism uses two app partitions (ota_0/ota_1) plus an `otadata` partition tracking boot state. If new firmware is flashed OTA but never explicitly marked "valid" (`esp_ota_mark_app_valid_cancel_rollback()`), or a self-test/rollback check isn't implemented, a botched update can leave the device in a boot loop with no way to recover — and since this device will be gifted and the developer explicitly does not plan to physically retrieve it, a bad OTA becomes permanent without a designed-in remote recovery path.

**Why it happens:**
Beginners implement the "happy path" of OTA (download + flash + reboot) without implementing the "did the new firmware actually work" confirmation step. The ESP-IDF rollback mechanism exists specifically for this but is opt-in — it requires deliberate implementation (self-test after boot, explicit valid-mark call, and CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) rather than being automatic.

**How to avoid:**
- Enable app rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`) and implement a genuine post-update self-test (e.g., LVGL initializes, wifi connects, core screens render) before calling `esp_ota_mark_app_valid_cancel_rollback()`.
- If self-test fails, explicitly call `esp_ota_mark_app_invalid_rollback_and_reboot()` to fall back to the last-known-good partition automatically — never leave firmware in `ESP_OTA_IMG_PENDING_VERIFY` indefinitely.
- Test the OTA update-fails-and-rolls-back path deliberately on the bench (e.g., flash a deliberately broken firmware) before ever shipping OTA to the gifted device.
- Keep OTA payloads small and gate the update on a stable Wi-Fi connection with retry/resume — a failed download mid-flash is also a bricking risk if not handled with checksums before the flash write is committed.
- Never disable rollback "to save flash space" — the two-partition scheme's whole purpose is this safety net.

**Warning signs:**
Any OTA test that doesn't include deliberately breaking the new firmware to confirm rollback actually happens; boot loops after any OTA in bench testing.

**Phase to address:**
OTA implementation phase — rollback/self-test must be built and proven (with a deliberately-broken test firmware) in the same phase the OTA mechanism is built, not deferred. This must be solid before Secure Boot + Flash Encryption are enabled (see Pitfall 5), since those remove most manual-recovery options.

---

### Pitfall 5: Secure Boot + Flash Encryption mistakes are irreversible and can permanently destroy the board

**What goes wrong:**
Enabling Secure Boot and/or Flash Encryption burns physical eFuses on the chip — a one-time, permanent, unrecoverable operation. In Release mode, once enabled, the device can never be reflashed without the (now-unreadable, fused-in) encryption key. A misconfiguration (wrong signing key, flashing plaintext data to an encrypted device, incorrect eFuse settings) can produce a boot loop with `flash read err, 1000` or `invalid header` errors that is permanent — the chip, not just the firmware, is bricked. Development-mode flash encryption allows disabling via the `FLASH_CRYPT_CNT` eFuse, but only 3 times total per chip, after which it's permanently locked either way.

**Why it happens:**
Secure Boot/Flash Encryption are treated as "flip a config flag and reflash," when they are actually a one-way, physically permanent hardware operation with no undo. First-time users often enable them on the actual target hardware to "get it done," rather than rehearsing the entire flow first on a cheap disposable dev board.

**How to avoid:**
- Practice the full Secure Boot + Flash Encryption enable/verify/OTA-update cycle on a cheap, disposable ESP32-S3 dev board first (not the final gift hardware) — treat eFuse burning as a dress rehearsal requirement, not something to do live on the real device.
- Use development mode first (allows some recovery via `FLASH_CRYPT_CNT`, max 3 attempts) to validate the whole build/flash/OTA pipeline before switching to Release mode (irreversible, no recovery).
- Double, triple check the signing key and eFuse configuration against the exact target chip revision before burning anything on the final board.
- Sequence matters: get OTA + rollback fully proven (Pitfall 4) BEFORE enabling Secure Boot + Flash Encryption, since after encryption is enabled, recovering from any latent OTA bug becomes much harder (no more plaintext reflashing via USB).
- This must be the very last hardware step before gifting, done only once the entire firmware (all 5 sections, wifi provisioning, OTA, camera, audio) is proven stable — because after this point, standard USB reflash recovery is gone.

**Warning signs:**
Any hesitation about "is this eFuse setting correct" is a stop sign — verify against Espressif's official docs and a disposable rehearsal board before proceeding on real hardware.

**Phase to address:**
Final security-hardening phase, explicitly the last phase before the device is gifted — must be preceded by full rehearsal on throwaway hardware and by a fully-proven OTA rollback mechanism.

---

### Pitfall 6: SD card corruption from power loss, wrong SPI speed, or bus sharing with the display

**What goes wrong:**
FAT32 (the filesystem SD cards need for ESP32 compatibility) is not power-loss-safe — if power drops or the device resets while a file is being written (e.g., a new photo, letter, or song syncing from the cloud), the filesystem structure itself can corrupt, potentially taking the whole card's contents with it, not just the one file. Additionally, when a display and SD card share the same SPI bus (common on QSPI boards where the SD slot is wired to a shared bus with separate CS lines), simultaneous access without careful CS/mutex handling causes read/write failures or corrupted data — indistinguishable at first from a bad card.

**Why it happens:**
Beginners don't realize FAT32 lacks journaling, so any write interrupted by power loss/reset/brownout is much more dangerous than on a modern journaled filesystem. They also don't realize a shared SPI bus needs explicit mutual exclusion (never talk to the SD card and the display at the exact same instant) — a subtlety invisible in the LVGL simulator, which has neither a real SPI bus nor a real SD card.

**How to avoid:**
- If the display and SD card share an SPI bus, use a proper mutex/semaphore around SPI transactions so display refresh and SD read/write never interleave mid-transaction.
- Never write directly to the SD card as the "hot path" — buffer incoming synced content, write with fsync/flush discipline, and design the sync protocol so an interrupted write leaves the old file intact (write-to-temp-then-rename pattern) rather than corrupting the original.
- Combine with brownout protection (Pitfall 7) — most "SD card corrupted itself" reports are actually a brownout/reset happening mid-write, not a bad card.
- Buy Class 10, name-brand microSD cards (fewer wear-related failures) and keep card capacity ≤32GB so FAT32 (not exFAT) applies natively — exFAT support is less consistently reliable across ESP32 SD libraries.
- Test explicitly by pulling power mid-write during development, not just "hope it doesn't happen."

**Warning signs:**
SD card errors/mount failures that correlate with recent reboots, power blips, or Wi-Fi transmit bursts; corruption "spreading" beyond the one file that was being written.

**Phase to address:**
SD card / album+music storage phase, and revisited in the cloud-sync phase (where files are written to SD from a network source, the highest-risk write path).

---

### Pitfall 7: Insufficient power supply causes brownout resets when camera + Wi-Fi + display run together

**What goes wrong:**
Wi-Fi transmit bursts are the single largest current spike on an ESP32-S3 (can approach 500mA momentarily when combined with camera/display active); if the power rail (from the LiPo/PMIC, or a weak USB cable during bench testing) sags under that spike, the chip's brownout detector fires and resets the device — which looks like "random crashes" rather than an obvious power problem, and is especially likely during the wifi-sync feature specifically because that's when Wi-Fi TX + display + potentially camera are most likely to be active simultaneously.

**Why it happens:**
Bench testing over USB with a low-quality cable, or an under-designed battery discharge path, doesn't provide enough instantaneous current headroom for Wi-Fi TX spikes on top of everything else already running.

**How to avoid:**
- Do not disable the brownout detector to "fix" resets — it's correctly reporting a real power problem; disabling it just turns a clean reset into unpredictable corruption/hangs.
- Use short, thick, good-quality USB cables during bench development (thin/long cables have too much resistance).
- Verify the PMIC (AXP2101) is configured to provide sufficient current headroom on the 3.3V rail for camera + display + Wi-Fi peaking simultaneously — check the actual battery's discharge current rating too (C-rating), not just capacity (mAh).
- Add bulk capacitance near the power input if using a dev/breakout stage before the final board (470µF+ electrolytic, plus a small ceramic in parallel) — though on the final Waveshare board this is likely already handled onboard; verify rather than assume.
- Specifically test the worst-case combo (camera capture + wifi sync active + display animating) as its own scenario, since that's most likely to trip a brownout in practice.

**Warning signs:**
Random resets that correlate with Wi-Fi connecting/transmitting, camera capture, or display-heavy screens — especially on battery power but not when USB-powered (or vice versa), which points at power-path/PMIC configuration rather than firmware bugs.

**Phase to address:**
Power management phase (initial) and again as a targeted stress test once wifi sync + camera + display are all implemented together (same integration milestone as Pitfall 2).

---

### Pitfall 8: BLE Wi-Fi provisioning failure can lock the gifted device out with no recovery path

**What goes wrong:**
ESP-IDF's BLE provisioning has a known behavior gap: if provisioning is attempted with wrong Wi-Fi credentials, the provisioning service in some configurations keeps running but becomes undetectable from the companion app's home screen, or previously-stored NVS credentials become temporarily unavailable until a reboot. If the girlfriend (non-technical end user) mistypes a Wi-Fi password during provisioning, or her home Wi-Fi changes (new router, new password) after the device is gifted, a badly-implemented provisioning flow can leave her with no clear way to re-enter provisioning mode — and the developer has explicitly said he won't take the device back to fix it physically.

**Why it happens:**
Provisioning demos/tutorials show the happy path (correct credentials entered once) and rarely test or design for the wrong-credentials / network-changed / re-provision-later cases, which is exactly the scenario a real long-term end user will eventually hit.

**How to avoid:**
- Design an explicit, discoverable, non-technical "reset/re-provision Wi-Fi" flow reachable entirely from the device's own touchscreen UI (e.g., a clearly-labeled button in Settings: "Trocar Wi-Fi") — do not rely on BLE-app-side recovery, since she won't have (or know how to use) a provisioning app.
- Handle the wrong-credentials case explicitly: on connection failure, re-open provisioning automatically (BLE/SoftAP back on) rather than silently stalling, and show a clear on-device message ("Não consegui conectar, tente de novo").
- Use provisioning Security Level 1 (Proof of Possession) at minimum — Security Level 0 (open) has no encryption and is a real risk since this touches her home network.
- Test the "wrong password entered, then corrected" and "Wi-Fi changed months later" flows explicitly on the bench, not just first-time setup.
- Since Wi-Fi credentials must never be hardcoded (per project constraint) and BLE provisioning is the mechanism, this on-device self-service re-provisioning UI is not optional — it's the only way she can ever change networks without the developer's involvement.

**Warning signs:**
Provisioning flow only ever tested with correct credentials on the first try; no UI path exists on-device to re-enter provisioning after initial setup.

**Phase to address:**
Wi-Fi/BLE provisioning phase — the "re-provision later" and "wrong credentials" flows must be explicit acceptance criteria in that phase, not just "happy path connects."

---

### Pitfall 9: The PC/SDL2 simulator hides real-hardware problems, giving false confidence

**What goes wrong:**
The LVGL SDL2 simulator (already in use per project context) validates UI logic, navigation, and layout — but it has no real SPI/QSPI bus, no real PSRAM timing, no real touch controller, no real camera, no real battery/PMIC, and no real Wi-Fi radio. Features that work perfectly in the simulator (smooth animations, correct touch mapping, fast rendering) can behave completely differently on hardware: frame buffer sizing that fits comfortably in simulator RAM may not fit or may be too slow in real PSRAM; touch coordinates need real-hardware calibration; concurrency issues between camera/display/SD/wifi simply don't exist in the simulator at all.

**Why it happens:**
The simulator is (correctly) chosen to enable pre-hardware development, but its role can silently expand from "validate what I can validate before hardware arrives" to "prove the app works," which it cannot do for anything involving real peripherals, timing, or power.

**How to avoid:**
- Treat simulator-validated work as "UI/logic validated," explicitly not "hardware validated" — keep a running list of what's still unverified (touch calibration, PSRAM buffer sizing/speed, camera timing, SD bus sharing, battery behavior, Wi-Fi coexistence) and schedule real-hardware verification for each as its own checkpoint once hardware arrives.
- When hardware arrives, budget an explicit "reality check" phase before continuing feature work: re-verify frame buffer size/placement against actual PSRAM speed, calibrate touch, and test each peripheral in isolation before assuming simulator-era code just works.
- Don't stack multiple untested-on-hardware assumptions before the first real-hardware test — bring up display, then touch, then SD, then camera, then audio incrementally rather than integrating everything before any hardware verification.

**Warning signs:**
Large blocks of feature work built entirely in simulator without any interim hardware checkpoints once hardware is available; assuming performance numbers (frame rate, load time) observed in simulator will transfer to hardware.

**Phase to address:**
The very first hardware-arrival phase should be a dedicated bring-up/reality-check phase (display + touch + PSRAM sizing), before resuming feature-phase work from the simulator-based roadmap.

---

### Pitfall 10: First-time embedded/soldering mistakes cause damage before firmware is even involved

**What goes wrong:**
For someone with zero prior embedded electronics experience, several purely physical/electrical mistakes can damage a board or component before any code runs: reversed battery polarity on a JST connector, static discharge (ESD) damage while handling boards/camera modules (camera FPC connectors and image sensors are especially ESD-sensitive), connecting a 5V signal to a 3.3V-only pin (SD card breakout, camera, GPIO), forcing a delicate FPC (flat flex cable) connector for the camera and damaging the fragile pins, or applying power with a short circuit present from a wiring mistake.

**Why it happens:**
These are the classic "nobody told me" mistakes that experienced hobbyists no longer think about but are invisible risks to a true beginner, and none of them are caught by software/simulator testing — they're physical.

**How to avoid:**
- Before first power-on of any new peripheral (camera, SD card breakout if separate, battery), double-check polarity and voltage against the datasheet/board silkscreen — 5 minutes of checking prevents most of these.
- Touch a grounded metal surface (or use an anti-static wrist strap, cheap and worth buying) before handling the bare board or camera module, especially in dry weather.
- Never force an FPC/FFC connector — these use a locking flap that must be gently lifted before inserting the ribbon and closed after; forcing it without releasing the lock bends/breaks the delicate contacts.
- Confirm every external module (camera, SD) is rated for 3.3V logic before wiring it to the ESP32-S3 — this board's peripherals are 3.3V, and accidentally routing 5V (e.g. from a poorly-chosen power source) into a 3.3V-only input can destroy it instantly.
- Do a visual continuity/short check (or use a multimeter) before applying power to any hand-wired addition, even on a mostly-assembled commercial board.
- Keep the first several power-on tests tethered to USB with current-limiting awareness (some USB ports/hubs offer basic overcurrent protection) rather than jumping straight to unregulated battery power.

**Warning signs:**
Board or camera module feels warm immediately on power-up (should not), no response at all from a component that was working before a rewiring session, magic smoke (literally — stop immediately and disconnect power if this happens).

**Phase to address:**
Applies throughout all hardware phases but is most concentrated in the very first hardware bring-up phase (unboxing, first power-on, first peripheral wiring) — worth an explicit checklist/safety pass before that phase begins, independent of any specific feature.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|-----------------|------------------|
| Skip OTA rollback self-test, just flash-and-hope | Faster to first working OTA | Gifted device can brick permanently with no recovery | Never — implement before first OTA to real hardware |
| DIY voltage-divider battery percentage instead of PMIC fuel gauge | Less integration work upfront | Inaccurate/misleading battery % erodes trust in a device meant to "just work" for her | Only acceptable as a very early placeholder, must be replaced before gifting |
| Hardcode Wi-Fi credentials "just to test faster" | Speeds up early dev iteration | Directly violates project security requirement; risk of shipping a build with credentials baked in | Only in throwaway bench-test builds that never touch the final gift device or its Secure-Boot-eligible binary |
| Skip SPI bus mutex between display and SD "it mostly works" | Simpler code | Intermittent, hard-to-reproduce corruption once camera/sync features add more contention | Never for shipped firmware |
| Test only in SDL2 simulator for multiple phases in a row once hardware exists | Faster iteration, no hardware round-trip | Accumulates untested-on-real-hardware assumptions that surface all at once, hard to debug | Acceptable pre-hardware; not once hardware is available for the given subsystem |
| Enable Secure Boot + Flash Encryption directly on the gift hardware without rehearsal | Saves buying a second dev board | Risk of permanently, irreversibly bricking the actual gift device | Never |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|--------------|-----------------|-------------------|
| Camera (OV5640) + PSRAM | Assuming default board config already enables PSRAM DMA for camera | Explicitly enable `CONFIG_CAMERA_PSRAM_DMA` and verify PSRAM clock/mode configured for max supported speed |
| Display (QSPI) + SD card (shared SPI) | No mutex around SPI transactions between the two peripherals | Guard all SPI transactions with a mutex/semaphore; never issue overlapping display and SD transactions |
| ES8311 audio codec (I2S) | MCLK not routed/enabled, silent or popping audio | Confirm MCLK = sample_rate × 256 is actually reaching the codec; verify WS/DIN pins wired and configured, not left floating |
| PMIC (AXP2101) battery management | Leaving charge-current/cutoff-voltage at power-on defaults not matched to the actual battery | Explicitly configure charge current and voltage cutoffs to match the purchased LiPo cell's datasheet rating |
| BLE Wi-Fi provisioning | Only testing correct-credentials-first-try path | Explicitly test wrong-credential retry and "re-provision after months" flows with an on-device UI entry point |
| OTA + Secure Boot/Flash Encryption ordering | Enabling Secure Boot before OTA rollback is proven reliable | Prove OTA rollback (deliberately-broken firmware test) before ever enabling Secure Boot/Flash Encryption |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|-----------------|
| Undersized/misconfigured PSRAM speed for LVGL frame buffer | Animations feel laggy or tear only once frame buffer is large enough to matter | Configure PSRAM to its actual max supported mode (Octal vs Quad); consider partial/double buffering sized to fit available bandwidth | Noticeable once full-screen animated transitions or camera live-preview are added |
| Camera + Wi-Fi TX simultaneously | Camera frames drop/corrupt specifically during active wifi sync | Prefer still-JPEG capture over continuous preview; avoid syncing large payloads at the exact moment of photo capture | Breaks specifically when "take photo" and "sync to cloud" phases overlap in time |
| Everything validated only in SDL2 simulator | Feature "works" in dev, fails/behaves differently first time on hardware | Real-hardware checkpoint per subsystem as it's integrated, not deferred to a big-bang integration at the end | Breaks the first time multiple real peripherals run concurrently, which the simulator can't model |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Wi-Fi credentials hardcoded in firmware source, even temporarily, in a build that could ship | Credentials leak if firmware is ever extracted, and directly violates the stated project requirement | Use BLE provisioning + NVS storage exclusively for any build path that could reach the gift device |
| Enabling Secure Boot/Flash Encryption without first proving the full OTA+rollback pipeline | A stable-looking device becomes permanently unrecoverable if a later firmware bug is bad enough to need reflashing | Sequence: prove OTA rollback first, do security hardening as the deliberately-last phase |
| Provisioning left at Security Level 0 (open, unencrypted) "since it's just for testing" and forgotten | Someone on the local network segment can intercept/inject during provisioning, on the home network of the actual gift recipient | Use PoP-based Security Level 1 minimum for any provisioning flow that could run in her real home |
| No token/auth check on the cloud backend sync endpoint, "trusted since only one device" | A single leaked device-side token or an unauthenticated endpoint could let anyone push/pull her private photos and letters | Per-device unique token + HTTPS only, exactly as the project's stated security requirement already specifies — make sure this is actually enforced server-side, not just planned |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-------------------|
| No self-service way to re-provision Wi-Fi from the device itself | She's stuck offline forever if her home Wi-Fi ever changes, with no way to reach the developer for a fix | Build an obvious, on-device "Trocar Wi-Fi" settings entry that re-opens BLE provisioning, tested end-to-end |
| Battery percentage that's inaccurate or jumps around (from a DIY voltage-based estimator) | She loses trust that the device "just works," may be caught with a dead battery unexpectedly | Use the PMIC's real fuel gauge, calibrated against actual charge/discharge cycles before shipping |
| Assuming stylus works like a "real pen" with capacitive touch precision | Frustration in the "cartas" (letters/handwriting) feature if it depends on stylus precision that the hardware can't deliver | Verify actual stylus behavior on hardware early and design the handwriting UI around what the touch panel can actually resolve |
| Silent OTA/sync failures with no on-device feedback | She has no idea new photos/letters/songs failed to arrive, or that an update is stuck | Show simple on-device status (last synced time, "atualização disponível," clear error state) rather than failing silently |

## "Looks Done But Isn't" Checklist

- [ ] **OTA updates:** Often missing the actual rollback self-test call — verify by deliberately flashing a broken build and confirming automatic rollback to the last-good partition happens.
- [ ] **Wi-Fi provisioning:** Often missing the wrong-credentials retry path and any on-device re-provision entry point — verify by testing wrong password entry and a "re-provision months later" scenario, not just first-time setup.
- [ ] **Battery/PMIC integration:** Often missing charge-current/voltage-cutoff configuration matched to the actual cell — verify against the specific battery's datasheet, not power-on defaults.
- [ ] **SD card writes (sync/photo capture):** Often missing power-loss-safe write patterns (write-to-temp-then-rename) and SPI bus mutual exclusion with the display — verify by pulling power mid-write during testing.
- [ ] **Secure Boot + Flash Encryption:** Often "enabled" without ever rehearsing the full flow on disposable hardware first — verify the entire enable → verify → OTA-update cycle has been proven on a throwaway board before touching the gift device.
- [ ] **Camera capture during active Wi-Fi sync:** Often only tested in isolation — verify photo capture quality/success specifically while a cloud sync is simultaneously in progress.
- [ ] **Touch/stylus behavior:** Often assumed rather than verified on real hardware — confirm actual stylus compatibility with the specific capacitive touch panel before building stylus-dependent UI.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|----------------|------------------|
| Bad OTA without proven rollback, device now boot-looping (Secure Boot NOT yet enabled) | MEDIUM | Physically retrieve device once, reflash via USB with known-good firmware, then fix and re-prove the rollback mechanism before any future OTA |
| Bad OTA after Secure Boot + Flash Encryption enabled (Release mode) | HIGH / possibly unrecoverable | Physical USB reflash is blocked by design; only options are hoping the rollback partition (if implemented and proven pre-encryption) auto-recovers, or accepting the device may need full hardware replacement |
| SD card corruption | LOW-MEDIUM | Reformat card, restore album/music/letters from the cloud backend (this is exactly why a cloud copy of synced content matters — treat the SD card as a cache, not the sole copy) |
| Wrong Wi-Fi credentials entered, device stuck disconnected, no on-device re-provision UI (design gap) | MEDIUM | Requires a firmware update (if OTA still reachable via a fallback AP mode) or physical retrieval — this is exactly why Pitfall 8's on-device re-provision UI must be built proactively rather than retrofitted |
| Battery over-discharged deeply (device left off for a very long time) | LOW | Most PMICs (AXP2101 included) recover from deep discharge via trickle charge; recharge slowly and check cell for swelling before resuming normal use |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|-------------------|----------------|
| Capacitive touch + stylus incompatibility | Hardware bring-up (touch) | Physically test a capacitive-tip stylus against panel before building stylus-dependent UI |
| Camera/display/SD/PSRAM bandwidth contention | Camera integration phase + concurrent-subsystems integration checkpoint | Stress test camera capture + wifi sync + display animation running simultaneously |
| LiPo battery/PMIC safety | Power management (battery) phase, early | Charge cell attended on non-flammable surface for first several cycles; confirm charge current matches cell rating |
| OTA bricking without rollback | OTA implementation phase | Deliberately flash broken firmware; confirm automatic rollback to last-good partition |
| Secure Boot / Flash Encryption irreversibility | Final security-hardening phase (last, pre-gift) | Full enable/verify/OTA cycle rehearsed on disposable dev board first; only then applied once to gift hardware |
| SD card corruption | SD/storage phase + cloud-sync phase | Pull power mid-write during testing; confirm write-to-temp-then-rename pattern and SPI mutex in place |
| Brownout resets under combined load | Power management phase + concurrent-subsystems checkpoint | Stress test camera+wifi+display together on battery power specifically, not just USB power |
| BLE provisioning lockout | Wi-Fi/BLE provisioning phase | Test wrong-credential retry and on-device re-provisioning UI explicitly, not just first-time happy path |
| Simulator-to-hardware gap | First hardware-arrival phase (dedicated bring-up/reality-check) | Explicit per-subsystem hardware checkpoint before resuming simulator-derived feature work |
| Beginner physical/wiring mistakes | First hardware bring-up phase | Safety checklist pass (polarity, voltage, ESD, FPC handling) before first power-on of each new peripheral |

## Sources

- [ESP-IDF OTA documentation (Espressif, official)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html) — HIGH confidence
- [ESP-IDF Flash Encryption documentation (Espressif, official)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/flash-encryption.html) — HIGH confidence
- [Developing a Firmware Rollback Mechanism for ESP32 Devices — Maker Gear Lab](https://makergearlab.com/developing-a-firmware-rollback-mechanism-for-esp32-devices-after-failed-ota-updates/) — MEDIUM confidence
- [Prevent flashing (bricking) ESP32 if secureboot/flashencryption enabled — FluidNC GitHub issue](https://github.com/bdring/FluidNC/issues/149) — MEDIUM confidence
- [LVGL Espressif Tips and Tricks (LVGL official docs)](https://docs.lvgl.io/master/integration/chip_vendors/espressif/tips_and_tricks.html) — HIGH confidence
- [esp-bsp esp_lvgl_port performance notes (Espressif ecosystem)](https://github.com/Lzw655/esp-bsp/blob/master/components/esp_lvgl_port/docs/performance.md) — MEDIUM-HIGH confidence
- [ESP32-S3 interfacing with OV5640, board reboots during camera init — espressif/esp32-camera issue #389](https://github.com/espressif/esp32-camera/issues/389) — MEDIUM confidence
- [Display and SD card sharing SPI bus on ESP32 — LVGL Forum](https://forum.lvgl.io/t/display-and-sd-card-have-trouble-sharing-the-spi-bus-on-esp32/15312) — MEDIUM confidence
- [Entire SD card corrupted on any VFS write — espressif/esp-idf issue #12073](https://github.com/espressif/esp-idf/issues/12073) — MEDIUM confidence
- [SD Card formatting causes corruption — ESP32 Forum](https://www.esp32.com/viewtopic.php?t=38189) — MEDIUM confidence
- [ESP32 and LiPo fuel gauge — Arduino Forum](https://forum.arduino.cc/t/esp32-and-lipo-fuel-guage/609416) — MEDIUM confidence
- [LiIon Battery Safety 2025 — University of Tennessee EHS](https://ehs.utk.edu/wp-content/uploads/2025/08/LiIon_Battery_Safety_2025.pdf) — HIGH confidence (institutional safety guidance)
- [ESP32 Brownout Detector Errors And Random Resets — Universal Solder](https://www.universal-solder.ca/troubleshooting-esp32-brownout/) — MEDIUM confidence
- [ESP32 Brownout Guide — EmbeddedPrep](https://embeddedprep.com/esp32-brownout-tutorials/) — MEDIUM confidence
- [Wi-Fi Provisioning documentation (Espressif, official)](https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/provisioning/wifi_provisioning.html) — HIGH confidence
- [BLE provisioning turns off after failed WiFi authentication — espressif/arduino-esp32 issue #4139](https://github.com/espressif/arduino-esp32/issues/4139) — MEDIUM confidence
- [I²S: ES8311 codec mic returns silent/constant samples — espressif/esp-idf issue #18621](https://github.com/espressif/esp-idf/issues/18621) — MEDIUM confidence
- [ESP-IDF i2s_es8311 example README (Espressif, official)](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/i2s/i2s_codec/i2s_es8311/README.md) — HIGH confidence
- [LVGL PC Simulator documentation (LVGL official)](https://lvgl.io/docs/open/7.11/get-started/pc-simulator.html) — HIGH confidence

---
*Pitfalls research for: ESP32-S3 + LVGL handheld personal device (RatimOS)*
*Researched: 2026-08-26*
