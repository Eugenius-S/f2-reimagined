---
id: 256
title: The bottom-edge wall shadow has no notched termination
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [096, 108]
reviewed: no
---

## Goal

At the vine wall beside the bottom-edge exit zone (view tile 28096), the building shadow ends cleanly beside the wall without a notched gap or stray lit patch. Apply the cause-based rule to every matching wall-shadow termination, not just this map.

## Context

- Task 096 recorded the owner's 2026-10-27 shot `runs/inbox/2026-10-27-bottom-edge-exit-zone.jpg`: the owner's circle marks a shadow flaw by the house, but does not identify the exact pixel or direction of the defect.
- The matched sheet `runs/096-corner-piece-sun-side/inbox-bottom-edge-before-after.jpg` shows the marked area and the classic comparison. Main and 096 are identical there apart from the pen fence post, so this shadow defect was not fixed by 096.
- Tasks 104 and 108 changed the camera stop at the map edge. Task 108 addresses the black strip below drawn ground and explicitly excludes the fly camera; neither task diagnoses or claims this wall-adjacent shadow termination.
- This is a follow-up for the outstanding shadow find, not the camera-boundary work or the corner-post split in 107/189. First reproduce and pin the exact affected pixels, map objects and shadow geometry against the original view; if the evidence shows it is already fixed, record that with matched captures and close without a code change.

## Acceptance

- Matched 3D and classic captures at view tile 28096, same hour, zoom and camera, plus a close crop of the owner-marked location in `runs/256-bottom-edge-wall-shadow/`.
- Identify the wall, shadow caster and exact shadow edge responsible before changing code.
- Fix the general cause across matching wall-shadow terminations. If the original capture shows no defect at the marked location, document the reproduction failure and leave code unchanged.
- The regression remains pixel-clean on unchanged views; changed views have references and diff images only after the relevant review. CTest and exporter tests pass for code changes.

## Out of scope

Camera bottom-stop behavior (104/108), corner post card splitting (107/189), new shadow features.

## Log

- 2026-11-12 gpt review of 096: The saved evidence records a notched building-shadow edge by the vine wall at tile 28096, hour 8. 096 and main are pixel-identical there except for the separate pen fence corner post. Exact wall/object ownership and the affected pixels were not marked, so reproduction and pinning are required before any fix.
