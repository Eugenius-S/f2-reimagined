---
id: 145
title: The 3D map edge matches the game, cut where it cuts, with no orange band
milestone: M1
status: blocked
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The map edge batch. The 3D view shows nothing at the map's edge that the
game's 2D picture does not: no orange, dithered, see-through band over the
ground before the black, and the edge is cut where the original cuts it.

## Context

- Batch: the map edge (2026-11-04 inbox sweep). Folded in:
  - the inbox line of 2026-10-21: the 3D scene cuts the map edge farther out
    than the game (kladwtwn, the bottom edge by the cornfield,
    `runs/inbox/2026-10-21-map-edge-cut.webp`, the red line is where the
    original cuts). First screenshot the exit zones of the original maps to
    learn where the game cuts, then fit the clean edges (016) to that;
  - the inbox line of 2026-11-03: an orange translucent wedge at the map
    edge in fly shots, the same band as below.
- Found by Claude, 2026-11-01, on main `b056099a` in the background test
  run: `tools/dev-snapshot.ps1 -Map kladwtwn -Still -Hour 8 -ViewTile 24678`
  (and `-Zoom 0.5`). In the upper right, next to the black beyond the map, a
  wide band of ground is tinted orange with a fine dither pattern and ends in
  a slanted straight line: `runs/inbox/2026-11-01-orange-map-edge.jpg`.
- The snapshot JSON shows `map_clip` enabled, `source: authored`. Suspects:
  the map clip's edge fade, a scroll blocker or exit grid drawn as a tinted
  plane, a tile outside the authored bounds. Find which first.

## Acceptance

- kladwtwn's bottom edge by the cornfield is cut where the original cuts it
  (a shot next to the original's).

- The cause in the Log.
- before | after of the same view in `runs/145-orange-map-edge/`, plus the
  game's own 2D picture of it (`-Classic`); the fix is a general rule, not
  a special case for this map.
- ctest passes; the scene regression changes only at the map edge.

## Log

- 2026-11-01 Cut from Claude's 137 captures.

### 2026-11-05 gpt
Done: Traced the orange band to translucent exit-grid markers: a matched diagnostic capture with exit-grid cards hidden removes the band while preserving the same map, view tile, zoom, daylight and authored clip. Exit-grid objects now remain in the picking data but render as pick-only cards, so the transition stays available without drawing the marker overlay. Added dev snapshot logging for the active EDG rects and an `F2CE_DEV_CLEAN_EDGES=off` comparison mode. Release build and CTest passed (20/20); the fixed matched capture is in `build/task145-after/`, compared with `build/task145-3d-24678/` and `build/task145-classic-24678/`.
Next: Keep PR #293 draft while CI runs on the updated branch; the completed direct captures and regression evidence are in `runs/145-orange-map-edge/`.
Check: Release build and CTest passed 20/20. The full regression checker passed all 106 captured spots after updating only the 46 pixel-changing references. `tools/check_runs_size.py origin/main` and `tools/check_public_content.py origin/main` passed. The completed klatoxcv control fly tour and two failed Kladwtwn tour attempts are described below.
### 2026-11-05 gpt: regression/evidence follow-up

Captured the full 106-spot regression set at `runs/regression/20261105-113616-167-43892`. Comparing decoded RGB pixels against the old references found 46 changed spots; visual review of the complete changed-highlight sheet shows the differences at translucent EDG marker regions. The 60 unchanged spots have their original PNG/JSON references restored, avoiding unrelated baseline churn. The changed references now represent the general 3D rule that EDG cards remain pickable but are not rendered as translucent overlays.

Added matched before/after and Classic contextual JPEG evidence to `runs/145-orange-map-edge/`, with camera, resolution, time, clip, and executable hash metadata. Classic uses the vanilla framebuffer at game time 288005, so it is contextual rather than pixel-identical to the 3D pair at 288000. CTest passed 20/20 before the regression sweep. Next: run the regression checker against all 106 captures after the targeted reference restore, then verify CI on the merged-main branch and finalize PR #293 acceptance.
### 2026-11-05 gpt: fly-tour status

The full Kladwtwn fly tour did not complete on the baseline side: it produced 32 of 58 expected views, reached `tour_step=31`, then the harness timed out after 30 seconds without progress. A targeted Kladwtwn route with the two edge-area spots completed all 6 baseline views, but the branch side produced only 2 of 7 expected images and then timed out at 144 seconds. No further Kladwtwn fly-tour retry is planned after these two consecutive failures; the direct matched static before/after evidence and 106-spot regression remain valid.

A complete control tour of klatoxcv did finish (5 spots across three elevations). It reported 0 of 5 changed spots; the cave roofs and walls in those views contain no visible EDG overlay, so this is a no-unexpected-change control and does not prove the Kladwtwn edge effect. Its generated `changed.jpg` is retained with the evidence. PR #293 remains draft pending owner review of the missing completed fly-tour proof.

### 2026-11-05 gpt: stop after two failed Kladwtwn tours

Blocked: the full and reduced Kladwtwn fly tours both failed to complete,
including the branch-side reduced run. No further tour attempt is planned.
The code, matched direct captures, and regression sweep are complete, but the
map-specific fly-tour acceptance remains unverified. Keep PR #293 draft.
Next: return to this task only if a bounded Kladwtwn tour route or a harness
fix becomes available.
Check: the failure summaries and run paths above; do not treat the completed
klatoxcv control tour as proof of the Kladwtwn edge.

### 2026-11-06 gpt: refreshed-main edge verification

Done: Rebased the existing branch on current `origin/main` (`6beaf53f`) and
confirmed the exit-grid marker remains pickable while its translucent card is
hidden. A fresh targeted fly tour of Kladwtwn tile 24678 completed at -45, 0,
and +45 degrees on both main and this branch; the difference sheet shows the
orange marker removed at the two affected views and no change at +45. The
three-angle before/after sheet is `runs/145-orange-map-edge/tour-current-main-edge.jpg`.
The same-tile classic 2D view and current 3D still capture are in
`runs/145-orange-map-edge/current-time-8/`; their sidecars record 288005 and
288000 game ticks respectively, so they are adjacent contextual views rather
than a pixel-matched comparison. The existing matched static before/after
captures show the same authored clip bounds and the marker removal. Release
build and CTest passed 20/20; scene-export tests passed 135/135. The prior
full and reduced fly-tour timeouts are superseded by this successful bounded
route, but the regression references were retaken in this PR, so
`reviewed: no` remains until the required independent review.

Manual review: inspected both `runs/145-orange-map-edge/regression-review-*.jpg`
sheets covering all 46 changed before/after references. The changes sit on
translucent exit-grid wedges at map boundaries; regular geometry remains.
The other 60 references are pixel-identical. Confirmed `pickOnly` objects are
still submitted by `picking.cc`, and the exit-grid stays in the pick list for
the normal transition path.

Next: Verify the rebased head's CI. The PR remains draft until its CI is green.
Check: the current-main tour sheet, matched capture sidecars, build logs, and
test results described above.

## Log

- 2026-11-07: reopened by the owner's find: "when the camera moves into the
  3D mode the map loses its clean edges, they go cell-by-cell like the first
  iterations." Suspects from a first code read: the clean-edge clip resets
  while a map transition or camera change walks through
  updateMapClip (scene_view.cc), or a warm-cache scene built before the
  edge cut exists. The owner's scene.log has no fresh "clean map edges
  disabled" lines, so the clip-failure path did not fire; needs a live repro
  with logging on the transition itself.
- 2026-11-11 claude (audit, folded in from the inbox line of 2026-11-09): at the artemple entrance the sides are black because the authored edge is about 704 px wide; the owner wants the map grown left and right (side clip planes and terrain) with pines planted, so a 21:9 screen sees forest.

- 2026-11-11 task 248 audit: the original edge verification is recorded, but the new Artemple entrance request needs code to extend side clip planes and terrain and plant pines for 21:9. That implementation is still missing.

### 2026-11-12 gpt
Blocked: the original orange band is already fixed by merged PR #293. The matched e0 view at tile 24678, Classic context, and regression review remain recorded in `runs/145-orange-map-edge/`. The current unresolved find is different: switching into 3D reportedly makes clean map edges appear cell by cell; no fresh transition capture or transition log identifies its cause. Artemple's authored edge is about 704 px wide, leaving black sides at 21:9; the requested side-plane and terrain extension with pines is not implemented.
Check: fetched `origin/main` at `e1fb3405`; the Kladwtwn tour could not start twice because the harness child PowerShell lacked `Get-FileHash` (once via the runtime PowerShell, once via Windows PowerShell 5.1 with module auto-loading unavailable). Stopped after two attempts as required. The older matched 24678 captures still document the orange EDG fix, not this transition or Artemple scope.
Next: split out an allowed task to capture/log the 3D transition and implement Artemple's widened side bounds, ground, and pines. Side-plane extension calls into `ce/src/scene_renderer/map_clip.cc`, which is outside this round's allowed files, and terrain/pine placement needs a bounded design. Do not mark done until the new scope has matching 21:9 evidence and the Kladwtwn transition has a cause.

### 2026-11-12 gpt: transition and Artemple diagnostics
Blocked: the existing orange EDG fix remains in main (#293), but this run did not identify the reported cell-by-cell edge change or implement the Artemple 21:9 extension. A scripted preferences toggle did enter 3D on Kladwtwn (`enabled=false -> true`); the captured metadata reports `scene_view=full`, `renderer=scene-gl`, and the authored map clip enabled. It did not preserve a transition log or a before/after sequence, so it does not establish whether edges change cell by cell. The 21:9 Artemple snapshot stopped because its scene cache was absent: the process log says `floor preview unavailable: floor.bin is missing or unreadable`, and `game/mod-cache/scene/artemple/0` does not exist. No Artemple image was captured.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` produced `build/fallout2-ce.exe`; `ctest --test-dir build --output-on-failure` passed 27/27. The local task #423 tour test passed with explicit PowerShell module imports, but the controller reports PR #423 CI still fails to resolve `Get-FileHash` on Windows, so that tool fix is not validated. The transition probe command was `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map kladwtwn -TransitionWatch -PreferencesProbe toggle3d -Set3d enabled=0 -OutputDirectory build/task145-transition-on -CaptureConsole -TimeoutSeconds 120 -StepTimeoutSeconds 30`; it reported `PREFERENCES_PROBE PASS toggle3d enabled false -> true`. The Artemple attempt was `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map artemple -Elevation 0 -ViewTile 17288 -ScreenWidth 3440 -ScreenHeight 1440 -Scale 2 -Hour 8 -Ambient 65536 -Still -OutputDirectory build/task145-artemple-21x9 -CaptureConsole -TimeoutSeconds 120`; it stopped at `step timeout after 30 s without progress` after `preview did not start`. The local PR #423 changes were discarded and are not part of PR #421.
Next: keep status blocked. The transition cause, an Artemple 21:9 capture with a valid scene export, terrain extension, and general pine placement remain unverified or unimplemented. PR #421 stays open.

### 2026-11-12 gpt: resumed after tour hash fix
Blocked: fetched `origin` and merged `origin/main` at `b7ecc384`; PR #423 is in the updated base. The Release build succeeded and CTest passed 27/27. The requested Kladwtwn transition still does not reproduce the reported cell-by-cell edge change, so no renderer code was changed.

Transition attempts and evidence:
- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map kladwtwn -TransitionWatch -PreferencesProbe toggle3d -Set3d enabled=0 -OutputDirectory build/task145-transition-r2 -CaptureConsole -TimeoutSeconds 120 -StepTimeoutSeconds 45` completed the preferences probe (`enabled=false -> true`). Its two snapshots both show Kladwtwn tile 24678 already in `scene_view=full`, and the run wrote no `devtransitions.log`; it does not capture the edge animation.
- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map klagraz -ExitProbeDestination kladwtwn -TransitionWatch -Set3d enabled=1 -OutputDirectory build/task145-transition-klagraz-to-kladwtwn -CaptureConsole -TimeoutSeconds 180 -StepTimeoutSeconds 45` passed the KLAGRAZ exit 22161 to KLADWTWN/e0 tile 19850 after 27 frames. `runs/145-orange-map-edge/kladwtwn-transition-watch.log` records 99 KLADWTWN arrival callbacks: the callbacks before the first 3D frame have `iso=1`, `available=iso-disabled`; the first 3D callback has `iso=0`, `available=available`. No per-cell clip values or `clean map edges disabled` reason are recorded. This map transition does not reproduce the reported visual effect or identify its cause.

The repaired tour ran the full KLADWTWN route (57 views per side) after exporting the map into the worktree cache. The first attempt passed all 57 `before` views but its `after` views were `scene_view=off`: the newly created `build/scene-cache` contained Artemple only. After exporting KLADWTWN, a second tour completed both sides and reported 19/19 changed spots. That comparison is invalid: its `before` cache is exporter 1.12.0 with a different `fallout2.cfg` hash, while the worktree cache is exporter 1.13.0. No third tour was run. The mismatched sheets remain under ignored `build/` and are not regression evidence.

Artemple evidence: `py -3 tools/scene_export/export.py --game-root E:\Projects\fallout2-ce-3d\game --map artemple --cache-root build/scene-cache` exported 10 files from 354 resources. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map artemple -Elevation 0 -ViewTile 17288 -ScreenWidth 3440 -ScreenHeight 1440 -Scale 2 -Hour 8 -Ambient 65536 -Still -OutputDirectory build/task145-artemple-21x9-main -CaptureConsole -TimeoutSeconds 180 -StepTimeoutSeconds 45` succeeded. The sidecar confirms 3440x1440 and authored bounds `[-25.041667,-23.062500,13.958333,13.437500]`; the JPEG in `runs/145-orange-map-edge/artemple-21x9-before.jpg` shows black side margins. One view establishes the margins, but not safe new terrain bounds or general pine placement. No Artemple code was changed.

Performance baseline: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/perf-check.ps1` exited 1. Kladwtwn e0 measured avg 53.6 FPS, 1% low 3.6 FPS, worst 387.3 ms, load 1400.7 ms. KlatoXCV e2 measured 72.5 FPS, 1% low 14.1 FPS, worst 277.3 ms, load 709.4 ms. New Reno and Vault City did not produce `frame_log.csv` or `devperf.log`; the tool recorded 30-second watchdog timeouts on some warmup/measured launches. This is the pre-change baseline only; no scene_renderer code changed, so there is no after measurement.

Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` succeeded; `ctest --test-dir build --output-on-failure` passed 27/27; the Kladwtwn and Artemple cache checks both reported `scenes present`; the Kladwtwn exit probe passed; the Artemple snapshot exited 0. Do not claim the cell-by-cell issue is fixed or understood. The valid same-exporter tour, exact transition reproduction, and enough Artemple evidence to place pines remain outstanding.
Next: keep PR #421 draft and status blocked. Resume the visual diagnosis only with a transition capture that records or visibly reproduces the edge change, and the Artemple extension only with matched 21:9 references that show the intended ground and pine distribution.
