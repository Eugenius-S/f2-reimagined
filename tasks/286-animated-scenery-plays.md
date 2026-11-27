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
- 2026-11-27 glm: the live-window round (the review's protocol): a separate visible stock install (build/stock-root, no owner game/saves/settings), the 239 harness without -Background, PrintWindow every 150 ms, Klatoxcv e1 view 22924 hero 19291. CLASSIC POSITIVE CONTROL CONFIRMED: the pool zone cycles a steady 2300 px per frame pair, the still-rock control zone exactly 0. THE 3D TEST THEN RAN POSITIVE TOO: the scripted Ctrl+F10 chord (waitms-based, hold/key/release) flips the live game into the 3D view at 1.5 s and the pool cycles at 779/968/669 px per pair, control 0. Verdict for the water/slime kind: moves in both engines, no bug; the earlier frozen numbers were fixture artefacts. Cadence detail (the 3D's 800 px vs the classic's 2300 px per pair, then quiet) needs a controlled-interval pass before any pace claim. The other kinds need this protocol per kind. Artifacts: runs/286-animated-scenery-plays/03/04 strips + the README's live round section; raw frames in build/walk-286/vis-slime-cl and vis-slime-3d. PR 618 stays draft; the kind table (the acceptance) still wants the remaining kinds shot this way.
- 2026-11-27 glm: the review correction applied to the live round. The first strips carried transition frames (frame 3 classic is the Temple of Trials: the --dev-new-game start passes through it before the dev target loads; the 3D's frame 3 was the loader, frame 4 black). Rebuilt from the stable ranges only: classic frames 5..16 at the actual 245-278 ms gaps - the pool zone reads a steady 2300 px on eleven of eleven pairs, the still-rock control 0 on every pair; 3D frames 7..14 at 249-305 ms - 779/968/669 px on the first three pairs after the 1.5 s chord, the control 0, then 42/2/1/45 px as the sparse samples straddle the discrete cycle. Verdict scoped to this pool only (klatoxcv e1, the goop pool by the barrel at view 22924): it animates in both engines, no bug; no pace or cadence claim, nothing about other water or kinds. Strips rebuilt (03/04), README rewritten, raw frames and mtimes kept in build/walk-286/. PR 618 stays draft.
- 2026-11-27 glm: the fire round. Classic positive confirmed on the kladwtwn woodfire (view 22958): the flame flicker clusters 1300-5400 px per pair, in bursts because the visible window pauses the world on lost focus (run-live's own header documents it). The 3D chord flips the live game into the scene view and the scene view is LIVE (the camp critters' idles blink at 400-500 px per pair in frame), but its camera is spawn-centred, so the woodfire sat outside the frame - the fire 3D verdict is blocked by the missing scene-camera control, no bug claim. Spawn-distance audit added to the README: modmain fire d=5 and newr1 fire d=7 are verifiable with the scene view as-is; kladwtwn fire d=26, the monitors (d=20..49) and the sfdock shoreline need the scene camera control first. The next rounds take the spawn-adjacent set one kind at a time by this exact protocol.
- 2026-11-27 glm: the method corrections from the night control round. The modmain fire pair (classic+3D visible) froze mid-run - the double map load steals focus and _GNW95_lost_focus pauses the world (thirteen zero-diff pairs recorded, no verdict drawn). The -Background offscreen re-run of the slime pair reproduces the positive control offscreen: the pool cycles 28-72 px per 150 ms pair, the control exactly 0 - recorded as a method fallback, not as the presented-frame evidence the review asks for. The working recipe recorded in the README: the 239 harness without -Background over the stock install, PrintWindow every ~150 ms nominal (245-305 ms actual), the stable ranges only, the ROIs per engine (the classic pools sit top-left, the 3D pools right-of-centre at x1100-1500 y400-780). Remaining kinds need either a focus-holding visible run or the owner's word on the offscreen evidence.
