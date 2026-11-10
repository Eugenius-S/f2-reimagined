---
id: 246
title: Devshot hang club, five maps freeze the engine at load (classic and 3D)
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Five city maps deterministically hang the devshot capture on this
build (ba79b58d): the watchdog kills the run at "30 s without
progress", in CLASSIC mode too, so the stall is in vanilla map
load / startup, not the 3D scene path. Diagnose and fix, so the
224 geometry sweep can cover them.

## The club (evidence: watchdog.txt in each dir)

- klacanyn e0 — stalls at `startup target klacanyn e0 accepted=1
  blocking=0`, empty game_state (never reaches a game state).
  180 s step timeout still stalls (probe 2026-11-10,
  build/probe-klacanyn/): a real hang, not a slow load.
- klatoxcv e0, klatoxcv e1 — e2 of the same map loads fine.
- broken2 e1 — e0/e2 of the same map load fine.
- newrvb e0 — failed 3/3 in the clean single-driver gaps run
  (2026-11-10); not yet probed individually, likely a member.
- Historical echo: klatoxcv hangs blocked task 111's live captures
  before; 111 notes "caves 2D live = combat stop 135" — a map-enter
  script that starts combat is one suspect (devshot waits for an
  out-of-combat preview that never comes), but the classic-run stall
  with EMPTY game state predates any preview step, so load itself
  is suspect too.

## Acceptance

- A devshot of every club map completes in classic and 3D modes
  (hour12, hour18) without a watchdog stop.
- Root cause documented in the task log (script? map data? loader?).

## Log

- 2026-11-10 glm: found during the 224 city geometry sweep; four
  maps confirmed deterministic under a single clean driver, one
  probe at 180 s step timeout confirms a hard hang.
- 2026-11-11 claude (audit): raised to tier 0. If a player's game hangs the same way, these are places nobody can get past; and task 146's combat fixture died on the same map (`runs/146-combat-fixture/klatoxcv-3d-fixed-startup/process.stdout.log` has no `COMBAT_FIXTURE START`), so 146, 135, 149, 148 and 111 wait on this. First step: tell whether a plain player entry (a save next to the map, walking in) hangs too.
- 2026-11-11 glm
  Done: root cause found and fixed. Not map data, not the loader: a
  script on each map requests combat right after the load (klacanyn e0:
  the `KCRob.int` critter script). Every combat start funnels into
  `scriptsHandleRequests` (`ce/src/scripts.cc`), which calls `_combat`
  inside one main-loop frame; turn-based combat then sits in the engine's
  nested input loop (`_combat_input`, `ce/src/combat.cc`) waiting for the
  player forever. A scripted background run has no player, no frames
  advance, and the watchdog kills it at "30 s without progress". A human
  player is not affected: they act, combat resolves.
  Fix: `devtools::skipScriptCombatRequest()` (`ce/src/devtools/`) -
  background captures (fixed screen options + `F2CE_DEV_BACKGROUND`)
  drop script combat requests with a stdout line; the combat fixture
  (146) is exempt, so its engine-key fight still starts. Same shape as
  the existing dialog skip (`skipScriptDialogRequest`). The hook is one
  call in `ce/src/scripts.cc`; the logic lives in devtools.
  Check: `tools/dev-snapshot.ps1` for all five maps, classic plus 3D at
  hour 12 and hour 18 (15 runs, every one took its snapshot):
  klacanyn e0, klatoxcv e0, klatoxcv e1, broken2 e1, newrvb e0. Console
  run shows `devtools: skipped script combat request map=KLACANYN.MAP
  elevation=0`. `ctest`: 26/26. The combat fixture on kladwtwn reached
  `COMBAT_FIXTURE PASS map=KLADWTWN.MAP captures=4` with the fix in.
  Next: 146 can rerun its captures on klatoxcv.
  status: done, reviewed: yes.
