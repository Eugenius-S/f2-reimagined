---
id: 259
title: Verify scenery rotation exceptions and true sides in flight
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [122]
reviewed: no
---

## Goal

Visually verify the turn gate from task 122 on real multi-direction scenery:
variant arts stay at their placed rotation, while true sides follow the fly
camera.

## Context

- Task 122's saved pairs show CAR4b at camera turns 0 and 90, but do not show
  the claimed cactus and shelf exceptions or the smaller scenery that turns.
- Task 125 reports the cola and shelf shots failed framing; its raw shots are
  outside the landed tree. The 0/43 Klamath tour does not cover the Den cases.
- This is an evidence task only. Do not change the turn rule or vehicle/model
  geometry. The Arroyo head and fallen vertibird are single-direction arts.

## Acceptance

- In a background test instance, capture the same placed cactus art with six
  variants at two opposite fly quarters. Confirm its in-game rotation frame
  remains the one assigned on placement instead of following camera azimuth.
- Capture a shelf damage-state art at two opposite quarters and verify it
  also remains at its placed rotation.
- Capture at least one true-side scenery art (cola machine, wall lamp, or
  Vault City window) at the same two quarters and verify its visible side
  follows the camera.
- Store matched JPEG pairs and JSON sidecars under `runs/259-scenery-turn-exceptions-evidence/`;
  record map, elevation, object/art, tile, camera turns, build commit and
  setting state. Keep each image under 1 MB.
- If no suitable placement can be reached, record the attempted map/object
  and exact blocker; do not claim the case verified.

## Out of scope

Code changes, vehicle models, unique single-direction objects, and changing
the task 122 rotation rule.

## Log

### 2026-11-12 gpt
Done: queued after task 122 review found no committed visual proof for the
cactus and shelf exceptions. Task 125 says its shelf framing failed; this
task asks for matched runtime evidence and a true-side control.
Next: capture the cases above in a background test instance.
Check: not run; evidence task.

<<<<<<< HEAD
- 2026-11-13 claude (review of gpt's run): the build blocker in PR #457 (Ninja `Permission denied`) is a locked file from a parallel build or a running test game, not a defect: rerun the build once nothing else holds `build/`, then retake the captures.

- 2026-11-13 claude: the cola hidden by the test hero is not a blocker: `tools/dev-snapshot.ps1 -HeroTile <a tile out of view>` moves the hero away.
=======
### 2026-11-12 gpt (blocked)
Done: Saved an exploratory matched pair for DENBUS2 elevation 0, map object 592 at tile 15488, PID 0x02000041 (Cactus), FID 0x02000032 (cac2000.frm), turns 0 and 180. The turn-180 view partly hides the cactus behind the roof and fence, so the rotation exception is NOT verified. The retained JPEGs and JSON sidecars say this explicitly. No code or game files were changed.
Blocker: The two cola framing attempts failed: DENBUS1 tile 16510 is too small and far away; BROKEN1 tile 12513 is hidden behind the foreground roof. A full map-object scan found no placements using the six damage-state scenery arts shelf001.frm through shelf006.frm, including the Den and New Reno maps; placed Shelves objects use different art. Reaching those frames would require a new map placement, outside this evidence task. The required Release build also failed twice at Ninja recompaction of build/_deps/sdl2-subbuild with Permission denied. No further build or capture attempts were made.
Next: Keep this task blocked until a current Release build can be made and a reachable shelf damage-state placement is available; then capture the shelf and a framed true-side control.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` (failed twice with the permission error); `py -3 tools/shapes/art_sides.py --game-root <game>` (lists shelf001..006 as six-direction art); map scan via ReadOnlyGameFiles over maps/*.map (no direct shelf001..006 placement); `tools/dev-snapshot.ps1` (scene available for the retained cactus pair, but no accepted shelf or cola pair). The retained cactus sidecars record CE build commit 3cde6966 and executable SHA256 057f6a6a0b7f3fe5ff8468b6779cf27eea9f302d364abc13b7603251b3ebce4c. Not checked: a Release build of current origin/main, the shelf damage-state rotation, or the true-side scenery rotation.
>>>>>>> 9d765d2bd (tasks: record blocked scenery rotation evidence)
