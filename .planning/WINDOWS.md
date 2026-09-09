---
schema_version: 1
open_count: 2
waived_count: 0
fixed_count: 0
total_count: 2
last_updated: 2026-09-09T18:13:50.357Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02.1 | unrun-verify | .pio/build/native_sim/program |  | Task 1 manual UAT (tap jogos>conexo, select 4, enviar, voltar, relaunch, same board) not interactively executed — no GUI input-simulation tooling (xdotool) available in the execution sandbox; covered instead by 29 automated unit tests across the storage and conexo engine suites | open |  | 2026-09-09T18:13:50.234Z |  |
| 2 | 02.1 | unrun-verify | src/storage/game_state.c |  | Task 3 manual UAT (delete/corrupt assets/save/conexo.bin, relaunch, confirm fresh board + error copy visually) not interactively executed — same tooling gap; corruption/validation logic is instead covered by 6 automated field-level tests plus a headless no-crash smoke run | open |  | 2026-09-09T18:13:50.357Z |  |

````json
[
  {
    "id": 1,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": ".pio/build/native_sim/program",
    "line": null,
    "description": "Task 1 manual UAT (tap jogos>conexo, select 4, enviar, voltar, relaunch, same board) not interactively executed — no GUI input-simulation tooling (xdotool) available in the execution sandbox; covered instead by 29 automated unit tests across the storage and conexo engine suites",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T18:13:50.234Z",
    "resolved_at": null
  },
  {
    "id": 2,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "src/storage/game_state.c",
    "line": null,
    "description": "Task 3 manual UAT (delete/corrupt assets/save/conexo.bin, relaunch, confirm fresh board + error copy visually) not interactively executed — same tooling gap; corruption/validation logic is instead covered by 6 automated field-level tests plus a headless no-crash smoke run",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T18:13:50.357Z",
    "resolved_at": null
  }
]
````
