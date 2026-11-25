---
id: 283
title: The roof opens when the hero is inside, in every kind of building
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
- 2026-11-26 glm: done. 38 buildings across every town (Arroyo, Klamath, Den, Modoc, Vault City, Gecko, Redding, Broken Hills, New Reno, NCR, San Francisco, Vaults 13/15, Mariposa, Navarro, Enclave, Ghost Farm), hero inside and outside, classic | 3D pairs each. The 3D roof state matches the original in all 128 captures; no finds. Confirmed: the classic hides roofs at the hero's screen point (a hero against a wall keeps the roof open in both engines); the SFSHUTL2 floor's UNUSED ART tiles draw in both engines (distinct from the one-sided CLOSED ART object defect of 329); NAVARRO e0 28694 is the south-edge gas station, matching in both. Evidence: `runs/283-roofs-open-over-the-hero/README.md` + 5 pair JPEGs.
