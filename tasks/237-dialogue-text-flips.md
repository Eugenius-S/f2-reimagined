---
id: 237
title: Dialogue replies keep their sharp text; text records no longer pile up
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: [236]
reviewed: no
---

## Goal

In dialogue the reply list stays in the thin sharp text in every presented
frame. It does not flip to the heavy pixel font.

## Context

Task 236 blamed "487 of 600 frames without screen records". That count
included the world and startup frames before the dialogue opens (present
frame 608 in the fixture), so it was a measurement artifact. In the open
dialogue the screen records stay at 9 before the fix too, static or with the
mouse moving, so the owner's flip was not reproduced directly.

What the probe did show: the cursor save-under copies a screen rectangle out
and back, and each restore was cloned into new records that never merged
back. With a mouse sweep over the replies the screen records grew 9 to 335 and
the whole layer to 688, frame rate fell to about 15 fps. The layer holds at
most 4096 records and drops the oldest first, so a long hover can push the
reply records out. That route to the flip is a hypothesis, not observed.

Fix: `TextLayer::copy` merges each clone into an existing record of the same
buffer, owner and run. Evidence is in `runs/237-dialogue-text-flips/`.

## Acceptance

- Mouse sweep over the replies, 1500+ dialogue frames: screen records stay
  at 9, no empty frame (was 9 to 335).
- Unit test: restored pieces rejoin their run.
- ctest passes.

## Out of scope

The real hover highlight redraw (option enter and exit) was not exercised by
the probe. The owner should hover replies in the live game and report.

## Log

- Probe measured, root cause found, fix and test added. Barter and pipboy
  snapshots were not obtained (the fixtures gave no UI screen); the message
  log snapshot is in `after-log.jpg`.
- 2026-11-08 claude: closed, merged in #361 (text records merge instead of piling up); the owner's bold flip was not reproduced, a new sighting goes to 193.
