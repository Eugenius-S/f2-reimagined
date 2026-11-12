---
id: 259
title: Verify scenery rotation exceptions and true sides in flight
milestone: M1
status: todo
owner: any
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
