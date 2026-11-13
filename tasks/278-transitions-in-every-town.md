---
id: 278
title: Leaving a map, stairs and the world map keep 3D in every town
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [147]
reviewed: no
---

## Goal

In every town after Klamath, each exit grid, each stair, ladder and elevator, and each arrival from the world map ends in 3D on the right scene with the camera on the hero.

## Context

- 147 covers Klamath and builds the drivers (walking onto an exit tile, the stairs driver, the world map out and in). This task runs them over the rest of the game.
- Elevators are new here: the Vault maps, the Military Base, the Enclave, Navarro.

## Acceptance

- A table in `runs/278-transitions-in-every-town/README.md`: every transition of every town, the arrival crop, and whether a 2D frame or the old scene flashed.
- Every defect fixed by a rule or cut as its own task with its crop.
- ctest and the pixel regression pass.

## Out of scope

The world map screen itself (it stays 2D).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
