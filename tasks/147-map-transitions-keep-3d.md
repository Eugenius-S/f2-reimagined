---
id: 147
title: Map exits, elevations and the world map come back in 3D
milestone: M1
status: doing
owner: glm
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every way the hero leaves a Klamath map, changes elevation or returns from
the world map ends on the new map in 3D: the right scene, no stale scene
from the old map, no long 2D frame, the camera on the hero.

## Context

- 010 gives a scene for any map and elevation; nobody has walked the
  transitions themselves since the 3D view stays on more often (135).
- Ways to cover: each exit grid between the Klamath maps (kladwtwn,
  klamall, klagraz, klacanyn, klaratcv, klatoxcv, klatrap), stairs and
  ladders between elevations (klaratcv, klatoxcv), leaving to the world map
  and entering a map from it, loading a save made on another map.
- The background run can travel (`-Tour`) and load maps; add what is
  missing to drive an exit grid or an elevation change, or explain in the
  Log why a step was checked by hand.

## Acceptance

- A table in the Log: each transition, what the first 3D frame on arrival
  shows (crop in `runs/147-map-transitions-keep-3d/`), and whether a 2D
  frame or the old scene flashed.
- Every defect found fixed by a rule, or cut as its own task with its crop.
- ctest passes; the regression stays pixel for pixel.

## Log

- 2026-11-01 Cut with the 146 pool.
- 2026-11-02 Added opt-in `--dev-transition-watch` instrumentation to record
  present mode, map/elevation, iso state and hero tile across a map identity
  change. Added an opt-in exit-grid probe to the background snapshot runner.
  These are diagnostic tools; no render policy or game content was changed.
- Runtime evidence, background instance, KLAMALL exit tile 28078 to
  KLADWTWN/e0 arrival tile 25142:

  | Transition | First arrival frame and result | 2D/old-scene observation |
  | --- | --- | --- |
  | KLAMALL/e0 -> KLADWTWN/e0 | First captured 3D frame has KLADWTWN/e0, hero tile 25142; comparison: `runs/147-map-transitions-keep-3d/klamall-to-kladwtwn-first-frames.jpg` | The captured first arrival present is black loading background with HUD, followed by 3D. This alone does not establish whether any old scene flashed or whether the loading duration is acceptable. No defect conclusion. |
  | KLAGRAZ/e0 -> KLADWTWN/e0 | Not reached | Probe selected exit tile 22161, target tile 19850/e0, but projected click at (463,232) was not pickable; no transition or arrival capture. |
  | Other Klamath exits, elevation changes, world map out/in, save on another map and reload | Not checked | No runtime evidence. |

- In the measured KLAMALL -> KLADWTWN sequence, watcher recorded 167
  presents between map identities. The last 2D callback was at 23962 ms and
  the first 3D callback at 24092 ms, 130 ms apart. The 2D callback is marked
  `iso=1` / `available=iso-disabled` during map loading; do not treat the
  measured loading callbacks or this black frame as acceptance of the
  "no long 2D frame" requirement, and do not change loading fades from this
  evidence alone.
- Capture stop condition reached after two unsuccessful generic exit-probe
  attempts. The first aborted while overlay sync was off; after fixing that
  probe guard, the second failed because the projected KLAGRAZ exit was not
  pickable. Stop further runtime capture until the probe can use the normal
  movement/picking path or a verified pickable exit.
- `ctest --test-dir build --output-on-failure`: 18/18 passed. This does not
  validate transition coverage or the pixel regression requirement.
- Status remains `doing`. Next: fix and validate the exit probe, capture each
  listed route/elevation/world-map/save-load path with first-frame sequence,
  determine whether any 2D or old-scene flash is actually visible, then run
  the pixel regression and update this table. No transition defect is yet
  confirmed or fixed.

### 2026-11-03 gpt continuation

Done: confirmed the two documented generic exit-probe attempts are the current
runtime limit. The task is already claimed as `doing` by `gpt`; no third probe
was run. PR #221 remains draft with the partial KLAMALL-to-KLADWTWN sequence
and its timing, not full acceptance evidence.
Next: continue to task 140 under the owner's two-attempt exception. Resume 147
after the exit probe follows the normal movement/picking path.
Check: existing PR CI passed; the transition table records the partial result
and explicitly leaves all other routes, elevations and world-map/save-load
paths unverified.

### 2026-11-03 gpt: evidence path cleanup

Done: removed absolute local paths from the captured stdout logs under `runs/147-map-transitions-keep-3d/`; the transition captures and timing are unchanged. The previous PR check failed only on these paths. The two failed exit-probe attempts remain the runtime limit, so no new capture was made. The transition evidence is still partial and the PR remains draft.
Next: continue with task 140 as recorded above; revisit 147 only when the exit probe can use a verified pickable route.
Check: `tools/check_public_content.py origin/main` and `tools/check_runs_size.py origin/main` pass after this commit.

### 2026-11-03 merge origin/main

Merged `origin/main` (`d8cff6b0`) into this branch. Resolved the `tools/dev-snapshot.ps1` overlap by preserving transition-watch/exit-probe completion checks together with main's scripted-input completion checks. PowerShell parser and `git diff --check` pass. Runtime transition coverage remains incomplete as listed above; PR stays draft.

### 2026-11-04 gpt: merge fresh origin/main

Merged `origin/main` at `3b288fd6` with no conflicts. The transition matrix remains incomplete and its single captured arrival does not establish flash behavior or all route coverage. PR #221 stays draft; no additional runtime attempt was made.

### 2026-11-04 gpt: merge current main

Fetched and merged `origin/main` at `8e135a73867b` into the existing task
branch; no conflicts. Pushed merge commit `b51c67952f2e4271e31af54a7f5608e6a2cdbaa9`.
The two documented exit-probe attempts remain the runtime limit; no new
capture was made. PR #221 stays draft and the transition matrix remains
incomplete.

Check: PowerShell parser, public-content check, runs-size check and
`git diff --check` pass. No new runtime or pixel-regression result is claimed.


### 2026-11-04 gpt: rebased on current main and blocked

Rebased the owned task branch onto `origin/main` at `f2ef1236` as requested.
Resolved the task-header and `tools/dev-snapshot.ps1` conflicts by retaining
the transition diagnostics and the main branch's scripted-input checks. The
two failed KLAGRAZ exit-probe attempts remain the limit; no repeat was made
without a probe-path fix. Task status is `blocked`, and PR #221 remains draft:
only the KLAMALL-to-KLADWTWN arrival is captured, with all other routes,
elevations, world-map and save/load transitions unverified.
### 2026-11-04 gpt: rebase CI handoff

The branch was force-pushed with lease at `78483130` after rebasing onto
`origin/main` `f2ef1236`. At the time of this note, GitHub still reports PR
head `fac7cd9c` and its active CI is for that pre-rebase commit, while the
branch API reports `78483130`. No new transition capture was made. The PR
remains draft because acceptance is incomplete.

- 2026-11-08 glm: the transition instrumentation and exit-grid probe landed as PR #221 (conflicts with the combat fixture resolved by keeping both). First-arrival evidence covers KLAMALL to KLADWTWN only; KLAGRAZ exit probes failed twice.

### 2026-11-12 glm: the KLAGRAZ blocker is dead on fresh main; two exits fixed; two route families still need their driver

Done: the 2026-11-04 blocker (the KLAGRAZ exit probe's click "not
pickable") does not survive the fixes below; the same probe now walks the
whole route on fresh main. Fixes, one rule each:
- ce/src/devtools/devtools.cc: the exit probe clicks the hex the ground
  pick names, not the tile anchor: the anchor round-trips to a neighbouring
  hex (the KLAGRAZ exit picked row+1), so the probe scans a small ring of
  offsets until tileUnderMouse names the exit's hex.
- ce/src/scene_renderer/picking.cc: the ground pick accepts any hex the
  map draws ground on (the clean-edges clip trims the drawn edge zones,
  and the exit grids live in them; a player has to be able to click an
  exit). The clip keeps governing the picture; only the pick widened.

The Klamath exit routes, background run, fresh main 8c310a9be plus these
fixes (each row: the probe's own log line, devprobe.log in the named run
folder):

| Transition | Result | Evidence |
| --- | --- | --- |
| KLAGRAZ/e0 exit 22161 -> KLADWTWN/e0 19850 | passed, 30 frames | build/probe-147d-klagraz/ |
| KLADWTWN/e0 exit 26347 -> KLAMALL/e0 29089 | passed, 29 frames | build/probe-147-kladwtwn-klamall/ |
| -Tour kladwtwn->klamall->klagraz->kladwtwn (the last stop through a save) | every arrival in 3D, the right map, the camera on the hero | runs/147-map-transitions-keep-3d/tour-arrivals-kladwtwn-klamall-klagraz.jpg |
| KLADWTWN/e0 -> KLACANYN | probe timeout at KLADWTWN (the hero walked away from the exit; the outside-probe stage) | build/probe-147-kladwtwn-klacanyn/ |
| KLAMALL/e0 exit 29282 -> KLADWTWN | probe defect: after the probe's teleport the mall's map script removes the kladwtwn-facing exit grids (192 grids left, none with map=9 at stage 501); the probe needs to WALK onto the exit tile like a player instead of clicking it | build/probe-147m-klamall/ |
| KLARATCV e0/e1/e2 -> KLATOXCV | no klatoxcv-facing exit grid on any elevation: the stairs are a ladder/script route, not an exit grid; the probe needs an elevation-change driver | build/probe-147-klaratcv-e0,e1,e2/ |

Checks: ctest 27/27; tools/regression unit tests OK;
tools/check_public_content.py clean; tools/perf-check.ps1 red as on main
(the known bars): avg 59.5/55.0/53.8/58.4 fps, load 1694/2312/1583/867 ms,
against the verified reference's avg 56.6/55.8/54.0/56.4 and load
1939/2153/1607/989: within the band. The pixel regression on the 193A
merge ran with 150 points 0 changed pixels; the pick change does not
render.

Not done yet: the remaining matrix rows (the klacanyn route's walk stage,
the mall route's walk-based probe, the stairs' driver, the world map
out/in, save on another map) and the arrival-crop table for every row.
Next: teach the probe to walk onto the exit tile (the player's mechanic),
which also covers the mall; a stairs/ladder driver; then the full table.

### 2026-11-24 glm: the world map out and back from KLADWTWN shows 3D first; the watch records the hand-off frame

Done: the route the camera note left open (`runs/316-windows-hotfix/camera-notes.md`:
walk onto exit grid 23454, return from the world map) is driven end to end by
the player's own input, as a scene script on a separate install. No game logic
is changed on this branch: the test exe (sha256
`1bc2799cdf14f3c3ab1181768c37534621b80bd00758affce761985ba81a954d`) is main
`1c893f5af` plus the three test-tool fixes below. The untouched exe cannot run
this route: its script driver has two defects (a script cannot act on the
world map, and a scripted click lands in the screen corner of a background
window), so the clean-main proof is this exe, not a byte-identical build.

Evidence is in `runs/147-map-transitions-keep-3d/world-map-return-kladwtwn/`:
the route script `route-kladwtwn-worldmap.txt`, the frame log
`world-map-return-kladwtwn-transition.log` and a four panel sheet
`world-map-return-kladwtwn-sheet.jpg` (start, world map, hand-off frame, first
local frame).

| Transition | First frame on arrival | 2D frame, old scene, loading screen |
| --- | --- | --- |
| KLADWTWN/e0 -> world map | The world map, 2D: every logged frame while it is up is `mode=2d ... iso=1` (sheet, panel 2) | No 3D frame under the world map flag (0 in the log); the block ends `first-3d-capture result=none superseded=1`. |
| world map -> KLADWTWN/e0, entering tile 25075 | Frame 1030: 3D, `kladwtwn/e0/gm0#3`, view full, scene available, read back from the back buffer: `world-map-return-kladwtwn-first-3d-frame.jpg` and `.json`; the preview sidecar beside it has hero 25075 and `view_tile` 24678, the engine's centre for that tile, the same as a fresh load at the spawn (sheet, panel 4). | One 2D present between the click and it (frame 1029, 34 ms after the last world map frame): the map is loaded and the hero already on 25075, and the screen is still the world map with the party on Klamath (`world-map-return-kladwtwn-handoff-frame.json`, sheet, panel 3). No loading screen, no local map in 2D, no old 3D scene. The first 3D frame comes 222 ms after the last world map frame (188 ms in the second run); the watched run's own capture is part of that time. |

Steps (twice on the final exe, the harness exiting 0 both times; the second
run numbers the two frames 1030 and 1031): `dev-snapshot.ps1 -Map kladwtwn
-Elevation 0 -HeroTile 23455 -ViewTile 23059 -GameRoot <isolated install>
-CaptureConsole -SceneInput route-kladwtwn-worldmap.txt -DevInputShots 12
-TransitionWatch`. The script waits 1.2 s, snapshots, clicks (640,220): the
exit grid 23454, one hex right of the hero, who walks onto it and the world
map opens 2.5 s later; snapshots; clicks (557,176), the party hotspot on
Klamath; 3 s later snapshots. The location was not yet marked visited after
the dev start, so the click enters Klamath's first map directly and no town
map opens. The hero returns at the map's own entering tile 25075, not at the
exit grid (the game's rule). Five runs in all gave the same frame sequence.

Test tools changed (no render policy, no game content):
- `ce/src/devtools/devtools.cc`: the world map runs its own loop inside the
  map transition, where the main loop's `devtools::update()` never runs, so a
  scene script could not act on it. `vanillaPresented()` pumps the script
  while the world map mode flag is set.
- `ce/src/dinput.cc`: a scripted mouse position was applied as a step from the
  physical pointer's last sample (the absolute sensitivity tracker), so a
  scripted click in a background window landed at (959,490). The tracker now
  resets when a scripted position applies.
- The transition watch did not see the hand-off frame: its key changed only
  at the first 3D frame. The key now ends with the map load serial (`#N`,
  `mapGetLoadSerial()` in `ce/src/map.cc`, bumped once the view and the hero
  are on the loaded map), so the loaded map's first presented frame opens the
  arrival block and gets the first 2D capture; a key change that follows
  before the first 3D frame joins that block. A transition a map load cuts
  short is written as `first-3d-capture result=none superseded=1` instead of
  being lost. `first-2d-capture` names its JSON. `first-3d-frame-capture` is
  a readback of the back buffer of the first 3D Full frame, the frame about
  to be shown; the older `first-3d-capture` is a static preview taken before
  that frame is drawn.

Found, not fixed (test tools): a snapshot JSON with non-ASCII text is written
in the game's CP1251 (the world map's map name), which is not valid UTF-8:
two of the twelve sidecars in each run. The readback JSON keeps the label
`view: vanilla-framebuffer` (it is not a static preview;
`tools/dev-snapshot.ps1` matches that label for `-Classic`, so it was left).

Not verified: entering a different map or elevation from the world map (this
route returns to the same map, so a wrong scene could not show by content;
only the camera and the hero moved), the town map path (T on a visited
location), travel across the world map and random encounters, and a visible
window with a real mouse (background runs only). `perf-check` was not run.

Next: the remaining rows (the klacanyn walk stage, the mall's walk-based
probe, the stairs driver, the world map into another map, a save made on
another map); then the full arrival table. Status stays `doing`.
Check: `ctest --test-dir build --output-on-failure` 34/34; tools unit tests
scene_export 177, regression 12, tour 18, shapes 15, tools 47 (1 skipped);
`tools/tasks.ps1 -SelfTest` and `-CheckMetadata` ok; `tools/test_snapshot_output.ps1`
54 checks passed; the route twice with harness exit 0.

The pixel regression is red on main's own build, and not from this branch.
`tools/regression.ps1` with this checkout's exporter (1.13.0) and the
separate install passes 29 of 154 spots against the committed references
(last changed 2026-11-08); the counts are in
`world-map-return-kladwtwn-regression-summary.txt`. The clean main exe
(`c0e10353`) and the branch exe give 0 changed pixels on 12 spots sampled
across the failing and the passing ones, with one cache and one install for
both, and both differ from the references by the same counts
(`world-map-return-kladwtwn-pixel-ab.txt`). The export read the same bytes
as the main checkout's game folder (its 15 archives, `ddraw.ini` and
`mods_order.txt` by sha256), so the separate install is not the cause. Why
the references no longer match main was not determined here.
