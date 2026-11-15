---
id: 283
title: The roof opens when the hero is inside, in every kind of building
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

When the hero stands inside a building the 3D view shows the room, not the roof, and the roof is back when the hero is outside.

## Context

- Nobody has swept this since the roofs batch (190). `tools/dev-snapshot.ps1 -HeroTile` places the hero; pick one room per building kind per town (a house, a shop, a tent, a cave mouth, a vault door, a multi-storey building).
- Compare with `-Classic` at the same tile: the original hides the roof of the building the hero is in.

## Acceptance

- Classic | 3D pairs, hero inside and hero outside, for at least 30 buildings across every town in `runs/283-roofs-open-over-the-hero/`, with a line per building in the README (right, or the find).
- Every find fixed by one rule; ctest and the pixel regression pass.

## Out of scope

Roof geometry seen from outside (190's finds).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
