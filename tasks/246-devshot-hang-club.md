---
id: 246
title: Devshot hang club — five maps freeze the engine at load (classic and 3D)
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
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
- 2026-11-10 glm, build-range observation: the 225 before-side tour
  (build b4b54575, BEFORE #320/#370) shot all club maps fine — its
  stamps for klacanyn e0, klatoxcv e0/e1, broken2 e1, newrvb e0
  reused cleanly on this run. The hang appeared somewhere in
  b4b54575..ba79b58d (#320 water corners, #370's cache+revert,
  #387..#395). Also on 2026-11-10: the BEFORE build itself stalled
  on sfelronb e0 (tour stop, 132 s timeout, twice in a row) — a
  sixth staller, and one that b4b54575 did NOT hang on in past
  runs. So the stall set shifts between runs/builds: treat as a
  load-time race, not a per-map data poison. Next probe: hang the
  loader under a debugger (thread dump at the stall), or bisect
  b4b54575..ba79b58d with a klacanyn e0 devshot probe (clear the
  probe output dir per step — stale devshot JSONs fake GOOD).
