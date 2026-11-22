---
id: 270
title: A 3D capture never comes out with white ground
milestone: M1
status: todo
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: [306]
reviewed: no
---

## Goal

Find why `tools/dev-snapshot.ps1` on KLAMALL e0 at view tile 28096 gave a 3D frame with white ground (256, PR #455), and fix the cause or make the run fail.

## Context

- 2026-11-15: PR #504 isolates the white floor to the bake-enabled renderer path. Task 306 supplies the renderer fix excluded below; resume this evidence check after it lands.

Both 3D attempts in 256 (`-Still -Shadows`, then `-NoPreview`) show white ground where the floor should be. Suspects: a stale or half-built scene cache for the map, a missing `tiles.png`, the capture taken before the floor texture is uploaded.

## Acceptance

- The cause is named in the Log with the evidence.
- The same command gives a frame with the floor, saved in `runs/270-white-ground-capture/`; or, if the cache is at fault, the harness rebuilds it or stops with a message.
- With that frame, say in 256's Log whether the notched shadow edge shows.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.
- 2026-11-17 claude: 306 fixed the renderer cause (the bake target's creation took the floor's texture unit, and the bake ran without the floor's light mode). The original KLAMALL command now gives a textured floor with the bake on: `runs/306-ground-bake-keeps-floor-texture/before-after.jpg`. This task's own evidence and 256's shadow check stay here.
