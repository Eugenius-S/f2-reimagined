---
id: 239
title: Overlays draw clean - no hex cursor halo, no black box on the edge cursor, the shot mark erased
milestone: M1
status: open
owner: glm
suggested: glm
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Two cursor faults the owner saw on 2026-11-08 (kladwtwn, windowed, scale 2,
3440x1440, sharp text on):

1. The red hex move cursor has dotted ghost outlines around it: one or two
   larger dashed rings of the same red, as if older or scaled copies of the
   cursor stay drawn. Crop: `runs/239-cursor-artifacts/hex-cursor-halo.jpg`
   (his shot `20261108-181235-kladwtwn-e0`).
2. At the map's top edge the scroll cursor (arrow with the red X, "cannot
   scroll") sits in an opaque black box: its transparent pixels draw black.
   Crop: `runs/239-cursor-artifacts/edge-cursor-black-box.jpg` (his shot
   `20261108-181209-kladwtwn-e0`).
3. Stacked here (the owner, 2026-11-08): the screenshot region mark
   (Ctrl+F12, task 183 step 2, 218) stays drawn on the bottom interface
   panel after the mark ends and is never erased. Same family as 1: pixels
   drawn over the interface or the 3D view that nothing clears.

Find why each happens, fix it at the root, and show before and after shots
from the test instance.

## Acceptance

- The hex cursor shows only its own outline, while moving and when still.
- The edge scroll cursors keep their transparency over the 3D view.
- After a region mark is saved or cancelled, the bottom panel shows no
  trace of it.

## Log

- 2026-11-08 claude: cut from the owner's shots.
- 2026-11-08 claude: stacked the region-mark find from the inbox (part 3); suggested glm.
- 2026-11-08 claude: owner order 2026-11-08: glm, first in its queue: 239, 172, 211, 212, then 224.
