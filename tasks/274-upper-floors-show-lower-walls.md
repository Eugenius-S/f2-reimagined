---
id: 274
title: An upper floor shows the tall walls of the floor below, as the original does
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
