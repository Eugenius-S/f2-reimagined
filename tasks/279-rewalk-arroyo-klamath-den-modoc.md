---
id: 279
title: Geometry re-walk: Arroyo, Klamath, the Den, Modoc
milestone: M1
status: done
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

Every spot of the listed maps is looked at with fresh eyes in the game camera and in a turned view, and each flaw is fixed or handed to its batch.

## Context

- A slice of 224 (the geometry second pass), cut by region so the walk has an end. Maps: Arroyo, Klamath, the Den, Modoc and their farms and caves.
- Spots: `tools/regression/spots.json` and `tools/tour/spots.json` for those maps. A find is pinned by spot, hex and frame before any fix.
- The owner's rule (2026-11-04): a fix is one rule for a kind of object across every map. A wall find goes to 189, a furniture or prop find to 191, each as a numbered line with its crop; a find of another kind gets its own PR here.

## Acceptance

- `runs/279-rewalk-arroyo-klamath-den-modoc/README.md`: one line per spot (clean, or the find: hex, what is wrong, frame).
- Every find fixed here, or written as a numbered line in 189, 191 or the inbox with the evidence link.
- No reference refresh without the owner's word; ctest and the pixel regression pass on each fix.

## Out of scope

Speed, lighting levels and the UI.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-25 glm: walk complete at main 12992e533 (26 maps, 109 spots, game camera and +45 fly turn each; every dark or odd cell re-checked at full resolution against the classic 2D view at the same tile). No new geometry defects: the upper-floor bare look of denbus1 e1/e2, denbus2 e1 and gstcav2 e1 matches the original 2D. arvill2 skipped: not in the worldmap registry, the dev tour cannot enter it (a fixture job of its own). Known finds re-confirmed on main: the Den chain-link fences (191 find 1, task 276 draft), flat cliffs edge-on in the fly view. Evidence: runs/279-rewalk-arroyo-klamath-den-modoc/ (README has a line per spot).
