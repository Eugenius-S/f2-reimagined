---
id: 212
title: The load dialog shows each save's preview picture
milestone: M1
status: done
owner: glm
suggested: glm
priority: medium
tier: 2
claude-only: no
user-check: yes
depends-on: [134]
reviewed: yes
---

## Goal

The owner, 2026-11-07: "there is no save preview when loading." The load
dialog must show the picture of the save slot being picked.

## Context

- The engine captures a preview at save time (a 640x380 area of the
  isometric window, loadsave.cc) and the load dialog draws it back.
- Suspects: saves made while the 3D scene was active capture the isometric
  framebuffer that the 3D presentation does not draw into, so the stored
  preview is black or stale; or the preview drawing path fails at the
  owner's current resolution/scale.
- Needs one headless repro on the demo copy: save with the 3D view active,
  open the load dialog, capture. Then fix whichever half loses the picture
  (capture from the presented frame at save time, or draw it at load).

## Acceptance

- A save made in the 3D view shows its picture in the load dialog.
- A save made in the classic view keeps showing its picture.

## Log

- 2026-11-07: owner report; the capture-at-save suspect recorded; repro
  pending.
- 2026-11-07, later: the owner is right that this was implemented - gpt's
  branch task/134-save-preview-3d-gpt (capture and reload 3D save previews,
  the evidence gap closed) never reached main. The old PR 297 sat draft
  with conflicts. Rebased onto current main (the shared readback helpers
  resolved in favor of the 183 versions, the preview capture kept as a
  thin wrapper), build verified, PR 297 refreshed and marked ready.
- 2026-11-08 claude: owner order 2026-11-08: glm, fourth after 211.
- 2026-11-10, glm. No new code was needed: the fix is #297 (the task 134
  recovery, merged 2026-11-07). Verified it on the current build: two full
  save→load round trips on the demo Klamath startup (1920x1080, scale 2,
  isolated test roots), one with the 3D scene active, one in the classic
  view. Both report SAVE_PREVIEW_FIXTURE PASS (save_ui=1 load_ui=1
  saved=1 loaded=1), and the load-dialog snapshots show the saved slot's
  picture in both modes - the 3D-presented street view for the 3D save,
  the 2D view for the classic save; neither black nor stale. Evidence:
  runs/212-save-preview-in-load-dialog/ (README, fixture-result.txt, the
  save and load screens of both runs). Done.
