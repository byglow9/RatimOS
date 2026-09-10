---
schema_version: 1
open_count: 7
waived_count: 0
fixed_count: 0
total_count: 7
last_updated: 2026-09-10T13:55:24.242Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02.1 | unrun-verify | .pio/build/native_sim/program |  | Task 1 manual UAT (tap jogos>conexo, select 4, enviar, voltar, relaunch, same board) not interactively executed — no GUI input-simulation tooling (xdotool) available in the execution sandbox; covered instead by 29 automated unit tests across the storage and conexo engine suites | open |  | 2026-09-09T18:13:50.234Z |  |
| 2 | 02.1 | unrun-verify | src/storage/game_state.c |  | Task 3 manual UAT (delete/corrupt assets/save/conexo.bin, relaunch, confirm fresh board + error copy visually) not interactively executed — same tooling gap; corruption/validation logic is instead covered by 6 automated field-level tests plus a headless no-crash smoke run | open |  | 2026-09-09T18:13:50.357Z |  |
| 3 | 02.1 | unrun-verify | docs/visual-identity/README.md |  | Task 3's human-check (launch the simulator, visually compare the icon/font distinctness table side-by-side with the developer's own colombiaOS reference photos) could not be executed by the agent -- no colombiaOS reference images are present in this sandbox/repo, and xdotool is unavailable to drive interactive input. Substituted with: a captured screenshot of the running native_sim home screen (icons + pixel-font sectionbar title render correctly, confirmed by the agent) plus the written provenance/distinctness table. The literal side-by-side photo comparison against colombiaOS needs a human with the reference photos. | open |  | 2026-09-09T18:41:36.620Z |  |
| 4 | 02.1 | unrun-verify | src/ratimos/apps/castelo_app.c |  | Interactive tap-through (home tile -> castelo screen -> play/win a game -> confirm day count) not executable in this sandbox (no xdotool/ydotool/xte). Substituted with unit tests + real screenshots of a crafted progression.bin fixture at two states; needs human UAT. | open |  | 2026-09-09T19:25:49.678Z |  |
| 5 | 02.1 | unrun-verify | src/ratimos/apps/jogos/sudoku.c |  | Task 2 manual UAT (open jogos>sudoku, fill cells, create a conflict, switch difficulty with the confirm dialog, navigate away/back, quit+relaunch, confirm board restored) not interactively executed -- no GUI input-simulation tooling (xdotool) available in this sandbox. Substituted with 20 automated engine/screen-adjacent unit tests plus a headless no-crash smoke run of the built binary; needs human UAT. | open |  | 2026-09-09T20:07:12.926Z |  |
| 6 | 02.1 | unrun-verify | src/ratimos/apps/jogos/sudoku.c |  | Task 3 manual UAT (select diario, solve the board via the engine's own solution, confirm resolvido!+1 no castelo banner, confirm castelo tile/conquistas update on home, re-enter and confirm no double-count) not interactively executed -- same tooling gap. Substituted with unit tests covering is_solved-from-own-solution, the daily-win-recorded guard (via test_storage_game_state), and a state round-trip test through the save blob; needs human UAT. | open |  | 2026-09-09T20:07:13.033Z |  |
| 7 | 02.1 | unrun-verify | src/ratimos/apps/jogos/termo.c |  | Task 3 manual UAT (play a guess and confirm per-letter colours, switch to quarteto and confirm 4 scrollable boards fit, navigate away and back, quit and relaunch, solve a daily session and confirm the castelo count) not interactively executed -- no GUI input-simulation tooling (xdotool) available in this sandbox. Substituted with 16 engine unit tests plus a headless no-crash smoke run of the built binary; needs human UAT. | open |  | 2026-09-10T13:55:24.242Z |  |

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
  },
  {
    "id": 3,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "docs/visual-identity/README.md",
    "line": null,
    "description": "Task 3's human-check (launch the simulator, visually compare the icon/font distinctness table side-by-side with the developer's own colombiaOS reference photos) could not be executed by the agent -- no colombiaOS reference images are present in this sandbox/repo, and xdotool is unavailable to drive interactive input. Substituted with: a captured screenshot of the running native_sim home screen (icons + pixel-font sectionbar title render correctly, confirmed by the agent) plus the written provenance/distinctness table. The literal side-by-side photo comparison against colombiaOS needs a human with the reference photos.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T18:41:36.620Z",
    "resolved_at": null
  },
  {
    "id": 4,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "src/ratimos/apps/castelo_app.c",
    "line": null,
    "description": "Interactive tap-through (home tile -> castelo screen -> play/win a game -> confirm day count) not executable in this sandbox (no xdotool/ydotool/xte). Substituted with unit tests + real screenshots of a crafted progression.bin fixture at two states; needs human UAT.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T19:25:49.678Z",
    "resolved_at": null
  },
  {
    "id": 5,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "src/ratimos/apps/jogos/sudoku.c",
    "line": null,
    "description": "Task 2 manual UAT (open jogos>sudoku, fill cells, create a conflict, switch difficulty with the confirm dialog, navigate away/back, quit+relaunch, confirm board restored) not interactively executed -- no GUI input-simulation tooling (xdotool) available in this sandbox. Substituted with 20 automated engine/screen-adjacent unit tests plus a headless no-crash smoke run of the built binary; needs human UAT.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T20:07:12.926Z",
    "resolved_at": null
  },
  {
    "id": 6,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "src/ratimos/apps/jogos/sudoku.c",
    "line": null,
    "description": "Task 3 manual UAT (select diario, solve the board via the engine's own solution, confirm resolvido!+1 no castelo banner, confirm castelo tile/conquistas update on home, re-enter and confirm no double-count) not interactively executed -- same tooling gap. Substituted with unit tests covering is_solved-from-own-solution, the daily-win-recorded guard (via test_storage_game_state), and a state round-trip test through the save blob; needs human UAT.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-09T20:07:13.033Z",
    "resolved_at": null
  },
  {
    "id": 7,
    "kind": "unrun-verify",
    "phase": "02.1",
    "file": "src/ratimos/apps/jogos/termo.c",
    "line": null,
    "description": "Task 3 manual UAT (play a guess and confirm per-letter colours, switch to quarteto and confirm 4 scrollable boards fit, navigate away and back, quit and relaunch, solve a daily session and confirm the castelo count) not interactively executed -- no GUI input-simulation tooling (xdotool) available in this sandbox. Substituted with 16 engine unit tests plus a headless no-crash smoke run of the built binary; needs human UAT.",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-09-10T13:55:24.242Z",
    "resolved_at": null
  }
]
````
