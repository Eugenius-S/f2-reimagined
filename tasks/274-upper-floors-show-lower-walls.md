---
id: 274
title: An upper floor shows the tall walls of the floor below, as the original does
milestone: M1
status: doing
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

On an upper elevation the 3D view shows the lower elevation's tall walls where the original 2D picture draws them.

## Context

- 224's find F2 (denbus2 e1): the original draws the lower elevation's tall walls into the upper view, the 3D upper scene does not. Evidence: `runs/224-geometry-second-pass/f2-denbus2-e1-2d-vs-3d-now.jpg`.
- One exporter rule for every map with stacked elevations, not a denbus2 case.

## Acceptance

- Classic | 3D pairs on denbus2 e1 and two other maps with a floor above walls, in `runs/274-upper-floors-show-lower-walls/`.
- Maps without such walls keep their scene files byte for byte (name the check).
- Exporter tests, ctest and the pixel regression pass; changed spots listed with the reason.

## Out of scope

Roofs (190) and the floors' own tiles.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-19 glm: the mechanism established with fresh classic-2D captures
  (dev-snapshot -Classic, DENBUS2.MAP elevation 1, verified in the sidecar):
  denbus2 e1's own data is 83 floor squares - a dirt strip at the map's
  north edge (rows 0-2), 0 objects, 0 roofs; everything else in the 2D view
  (street, building, walls) is elevation 0's scene the game draws past the
  upper floor's boundary. The 3D upper scene had none of it. Exporter rule
  (f91b25bb): an upper elevation's scene carries the lower elevations' wall
  objects, drawn first like the lower scene is, VISUAL ONLY - their faces
  never join this floor's blocking or light (the original upper floor keeps
  its own walkability and light grid). Serials name the combined list; the
  sidecars take it from the builder (export.py). denbus2 e1: 0 -> 833 wall
  rows. Byte-for-byte: without lower walls nothing changes - denbus2 e0's
  game-read files (floor/walls/baked/shadows/projection.bin, all png) are
  byte-identical across the rule, scene.json differs only in the
  exporterVersion stamp (bumped 1.13.0 -> 1.14.0; the game-side expected
  version constant rides PR 540 and needs the rebase after 540 lands).
  Tests: tools/scene_export/test_lower_walls.py (3), scene_export suite
  151/151.
