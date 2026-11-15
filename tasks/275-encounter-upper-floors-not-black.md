---
id: 275
title: Encounter maps' upper floors are not black in 3D
milestone: M1
status: todo
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

The upper elevations of the desert and coast encounter maps show in 3D what the original shows, not a black screen.

## Context

- Folded into 173 on 2026-11-05 and cut out here: desert5 e1 and desert6 e1 export with no floor art and no sprites (`walls.bin` 24 bytes, empty `tiles.png`, `indoor=true`), the 2D mean brightness is 46 against 0.8 in 3D. Shots: `runs/169-encdesertcoast-in-3d/finds/`.
- 271's table lists every elevation over 20% black: fix the whole class, not two maps.

## Acceptance

- Classic | 3D pairs for desert5 e1, desert6 e1 and every other elevation of that class in `runs/275-encounter-upper-floors-not-black/`; the 3D mean brightness within a third of the 2D one.
- An elevation the exporter still cannot build stands on the plane fallback (173's rule), never black.
- Exporter tests, ctest and the pixel regression pass.

## Out of scope

Cave lighting levels (262, 263).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
