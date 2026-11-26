---
id: 265
title: Capture the remaining multipart groups from the live game
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

Capture the two multipart groups left without live evidence by task 174: New Reno signage and the San Francisco tanker.

## Context

- PR #274 grouped multipart map objects in 3D and saved matched sheets for the Klamath and Navarro Vertibirds, the New Reno Vertibird, and the New Reno casino entrance.
- The `NEWR1` signage group at tile 10896 and the `SFDOCK` tanker at tile 8896 each timed out on two background capture attempts. Do not repeat those attempts with the same method.
- Task 192 covers the San Francisco tanker's floating hull sliver, not live proof that its multipart machinery stays together. Task 224's Shark Club marquee finding is on `newr2`, not the `NEWR1` signage group.

## Acceptance

- From the owner's live game, capture each target at fly turns -45, 0, and +45, with the map, elevation, tile, build, resolution, logical resolution, and scale recorded.
- Save one readable evidence sheet and metadata sidecar per target under `runs/265-multipart-evidence-followup/`.
- Confirm whether each group stays together in all three turns. If a group does not, record the specific defect and open a separate fix task.

## Out of scope

Changing multipart grouping code or retrying the timed-out background capture method.

## Log

### 2026-11-12 gpt
Done: created this owner capture follow-up after reviewing task 174's evidence and checking tasks 192 and 224 for exact coverage.
Next: owner captures the `NEWR1` signage and `SFDOCK` tanker from the live game.
Check: task 174 Log, task 192 Log, and task 224 evidence register.

- 2026-11-13 claude (review of gpt's run): plain captures, any agent can take them with the background test game; owner set to any. The owner's live game is not needed.

- 2026-11-27 gpt: the isolated stock MAP data locates both targets on e0.
  NEWR1 tile 10896 has serials 229 and 230 (renosgn6.frm and renosgn4.frm),
  both part of a solid in the warm scene. SFDOCK tile 8896 has tanker
  serial 9 (TANKER07.FRM). Use a standalone background fly tour for each
  target at -45, 0 and +45. Preserve the old live-attempt evidence.

- 2026-11-27 gpt: captured isolated stock background still tours at e0 on Windows Release `e14e2e75` (executable SHA-256 `6a04273eeef7434bb85bccecd32906bbd119a816090f8c996cf0ab4d5f40958f`), 1920x1080, logical 960x540, scale 2, `scene-gl`. Evidence: `runs/265-multipart-evidence-followup/newr1.jpg` and `sfdock.jpg`, with JSON sidecars. Each tour requested -45, 0 and +45 degrees at distance 30. At distance 8 the sign was cropped and the tanker camera was inside geometry; these raw runs remain ignored in `build/265-raw/`.
  NEWR1 tile 10896: the sign appears together at 0 and +45, but is nearly edge-on at -45, so its seams cannot be checked there. SFDOCK tile 8896: the tanker is clipped by the frame and the -45 camera is obstructed; the full multipart group and seams cannot be judged at any of the three angles. Visible isolated hull pieces resemble the already tracked task 192 sliver; these captures do not establish a new grouping defect. The static camera field in snapshot JSON does not encode fly turn; the requested tour stops and visibly different frames establish the order, not independently measured angle metadata. Neither target meets the three-angle acceptance. Next: select a suitable camera target and distance that frame the complete group, verify the angle from runtime metadata or another independent mechanism, then inspect all seams; preserve these captures.

- Blocked: two isolated runs produced partial views at distances 8 and 30.
  Both sheets were inspected. Use a suitable independent fly fixture to
  center the complete groups and capture the obscured joins before making
  a geometry decision. Keep the existing tanker sliver report in task 192.
