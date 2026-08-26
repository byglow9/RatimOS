---
phase: 1
slug: shell-storage-api-simulator-first-app-shells
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-08-26
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO's built-in Unity test runner for `platform = native` |
| **Config file** | none yet — no `test/` directory exists in this repo currently |
| **Quick run command** | `pio test -e native_sim -f test_storage` |
| **Full suite command** | `pio test -e native_sim` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pio run -e native_sim` (build must stay green) + `pio test -e native_sim -f test_storage` once that suite exists
- **After every plan wave:** Run `pio test -e native_sim` (full suite) + manual click-through of all 5 sections + splash observation
- **Before `/gsd-verify-work`:** Full suite green, manual UAT script (splash timing/no-skip, 5-section round-trip, storage-backed content visible in each app)
- **Max feedback latency:** ~5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-TBD | 01 | 1 | SHELL-01 | — | N/A | manual/smoke | `pio run -e native_sim && .pio/build/native_sim/program` (manual click-through) | ❌ W0 (stays manual — no LVGL headless test harness) | ⬜ pending |
| 01-01-TBD | 01 | 1 | SHELL-03 | — | N/A | manual/smoke | same manual run, observe splash duration/behavior | ❌ W0 (stays manual — visual timing/fade cannot be unit-tested) | ⬜ pending |
| 01-01-TBD | 01 | 1 | (supporting) Storage API returns correct counts/titles per domain from `assets/mock/` fixtures | T-1-01 | Fixture paths built from a fixed, compiled-in manifest — never directory-scan-and-trust or string-concatenation from a dynamic source | unit | `pio test -e native_sim -f test_storage` | ❌ Wave 0 — needs `test/test_storage/test_storage.c` | ⬜ pending |
| 01-01-TBD | 01 | 1 | (supporting) `board_waveshare_s3_35` stub compiles standalone | — | N/A | build/syntax-check | `gcc -fsyntax-only -std=gnu11 -Isrc src/board/waveshare_s3_35/board.c` | n/a — ad-hoc command, no file needed | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*Task IDs are placeholders — the planner fills in real task IDs when PLAN.md is written.*

---

## Wave 0 Requirements

- [ ] `test/test_storage/test_storage.c` — covers the Storage API's list/get functions against `assets/mock/` fixtures (pure C, no LVGL dependency, genuinely unit-testable)
- [ ] No `unity_config.h` customization needed — PlatformIO's default Unity config is sufficient
- [ ] Framework install: none — PlatformIO's native unit-testing support requires no additional `lib_deps` beyond the existing host GCC toolchain `native_sim` already depends on

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Home → 5 sections → back navigation | SHELL-01 | No LVGL headless/mock-display test harness exists or is proposed for this phase | Build+run `native_sim`, click through jogos/musica/album/cartas/config from home and back, confirm each returns to home |
| Splash shows before home, real progress, no tap-to-skip | SHELL-03 | Visual timing + fade animation cannot be meaningfully unit-tested without a virtual display | Launch simulator, observe splash fade-in, confirm bar progresses over ~2s, confirm tapping during splash does nothing, confirm home screen loads after |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
