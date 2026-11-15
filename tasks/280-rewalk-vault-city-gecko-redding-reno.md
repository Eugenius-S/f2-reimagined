---
id: 280
title: Geometry re-walk: Vault City, Gecko, Redding, Broken Hills, New Reno
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

Every spot of the listed maps is looked at with fresh eyes in the game camera and in a turned view, and each flaw is fixed or handed to its batch.

## Context

- A slice of 224 (the geometry second pass), cut by region so the walk has an end. Maps: Vault City, Gecko, Redding, Broken Hills, New Reno and its outskirts.
- Spots: `tools/regression/spots.json` and `tools/tour/spots.json` for those maps. A find is pinned by spot, hex and frame before any fix.
- The owner's rule (2026-11-04): a fix is one rule for a kind of object across every map. A wall find goes to 189, a furniture or prop find to 191, each as a numbered line with its crop; a find of another kind gets its own PR here.

## Acceptance

- `runs/280-rewalk-vault-city-gecko-redding-reno/README.md`: one line per spot (clean, or the find: hex, what is wrong, frame).
- Every find fixed here, or written as a numbered line in 189, 191 or the inbox with the evidence link.
- No reference refresh without the owner's word; ctest and the pixel regression pass on each fix.

## Out of scope

Speed, lighting levels and the UI.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
