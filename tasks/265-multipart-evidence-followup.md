---
id: 265
title: Capture the remaining multipart groups from the live game
milestone: M1
status: todo
owner: any
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
