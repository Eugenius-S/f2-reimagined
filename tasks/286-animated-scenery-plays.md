---
id: 286
title: Animated scenery moves in 3D: fires, water, screens, signs
milestone: M1
status: todo
owner: glm
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every scenery object and floor tile that animates in the original also animates in 3D, at the same pace.

## Context

- List the animated scenery from the art (frame count over one) and the palette-cycled colours (water, slime, fire, monitors, alarm lights); pick one map per kind.
- Two captures a known time apart must differ where the classic captures differ. `-CyclePalette` and `-HoldSeconds` exist in `tools/dev-snapshot.ps1`.

## Acceptance

- A table in `runs/286-animated-scenery-plays/README.md`: each animated kind, a map, moving in classic (yes or no), moving in 3D (yes or no), with the frame pairs.
- Every kind still in 3D but moving in the original is fixed by a rule, or pinned with its cause.
- ctest passes; perf-check before and after on any renderer change.

## Out of scope

Critter animations and combat effects.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-27 glm: took the card (free, no duplicate PR; 265 left alone, 271 on stop). Plan: the static inventory first, from the game data only - the palette's colour cycles (pal.py) against each representative map's scenery pixels, plus FRM frame counts - one map per kind (fire: kladwtwn's barrels and Modoc's braziers; water: sfdock; slime: klatoxcv; monitors: vault13; signs: newr1). Then Classic/3D pairs, one map/tile/mode at a time, -CyclePalette/-HoldSeconds, movement measured inside the object's own region.
- 2026-11-27 glm: the inventory is done and committed (runs/286-animated-scenery-plays/README.md): palette kinds - fire (barrel/woodfire/atorch on kladwtwn/modmain/sfdock/newr1/vault13), slime (goop01-04 on klatoxcv and sfdock), monitors (monitr3-8, comp1-6 on vault13/newr1/sfdock), shoreline (sfdock's seas tiles and shore props), alarm (1 px on comp3/4); frame-animated ambient - FLIES01/02 (10), SMOKE01 (12), steam01 (12), POWER001 (15), v13secur/2 (15), TANKERW1 (6), curtain (8), MODWELL1 (3); the door FRMs are the interactive class (284). The 3D already carries the cycle marks (alpha 252/254, task 022).
- 2026-11-27 glm: METHOD NOT CONFIRMED, the pairs are not taken. Four protocols failed to see animation in a background run: (1) still + -HoldSeconds - the hold runs after the snapshot and the still fixture freezes the world at a fixed tick; (2) owner-shot delays 1 s/4 s - identical frames, the sidecar game_time pinned at 302401/302402 in every run; (3) -NoPreview - the startup still labels the run static-wall-plus-hero preview and the clock stays pinned; (4) the 239 live-window harness (run-live.ps1, PrintWindow every 150 ms, -Background) - the window runs but the world update starves: the classic's game clock advanced 1 tick in 14 s, the cursor blinker moves while the slime pools measure 0 across consecutive pairs, and a delivered Ctrl+F10 chord (the input steps logged) did not flip the captured view to the 3D scene. The positive control the review asked for (klatoxcv e1 t22924, hero 19291, five phases per engine) showed zero movement on the classic side, so per the criterion nothing can be claimed about the 3D - neither moves nor static. Evidence: runs/286-animated-scenery-plays/ (the two frozen-pair sheets; raw frames in build/walk-286/).
- 2026-11-27 glm: status back to todo with the missing fixture named per the two-failures rule. The pairs need a capture from a live world: the 239 harness without -Background (a visible focused window) or a small fixture that lets the world update in an offscreen window; then protocol 4 as written (one run per engine, ~12 s, frames every 150 ms, diffs inside the pool's bbox). The focused-run freeze in _GNW95_lost_focus is real and documented in run-live's own header, so the visible run must keep focus throughout. No renderer, exporter or scene change shipped: nothing to ctest or perf-check this round; the PR carries the inventory and this method verdict only.
