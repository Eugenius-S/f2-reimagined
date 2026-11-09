---
id: 243
title: The exporter stops baking a multi-piece building into the floor; its cards cover the base
milestone: M1
status: done
owner: any
suggested: claude
priority: normal
tier: 2
claude-only: no
user-check: yes
depends-on: [241]
reviewed: no
---

## Goal

A composite sprite (one building cut into several art pieces, task 241's
group) is drawn by its own cards over its whole picture, so the whole
building takes the group's one sun verdict. Today the exporter bakes the
building's lower half into the floor atlas, and that half shades per pixel
like the ground.

## Context

- Found in 241: on artemple e0 the Temple of Trials' stairs and lower domes
  sit in the floor atlas `build/scene-cache/artemple/0/tiles.png` (the dark
  stone cells). The map's own floor tiles there are plain desert
  (edg5000..edg5007), so the stone comes from the exporter, not the game.
- With the floor draw skipped, the TEMPLE01..08 cards and walls.bin rows
  cover only the upper part. The rest is the floor's baked picture.
- The exporter lives in `tools/scene_export` (floor atlas:
  `floor_sidecar.py`, object modes: `baked_sidecar.py`). The renderer
  groups pieces by art set in `groupCompositeSprites`
  (`ce/src/scene_renderer/live_objects.cc`).

## Acceptance

- artemple at hours 9, 12 and 16: the temple, stairs and lower domes
  included, is all lit or all shaded; before and after shots in
  `runs/243-composite-not-baked-into-floor/`.
- The game's own view stays pixel for pixel (the regression), ctest passes.
- The caches are re-exported for every map the change touches.

## Out of scope

Per-pixel cast shadows across a building's face.

## Log

- 2026-11-09 claude: cut from 241 (the floor-baked base, the controller's option C).
- 2026-11-09 claude: folded into 224 on the owner's word (one object is not a task of its own); closed here.
