---
id: 286
title: Animated scenery moves in 3D: fires, water, screens, signs
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

Every scenery object and floor tile that animates in the original also animates in 3D, at the same pace.

## Context

- List the animated scenery from the art (frame count over one) and the palette-cycled colours (water, slime, fire, monitors, alarm lights); pick one map per kind.
- Two captures a known time apart must differ where the classic captures differ. `-CyclePalette` and `-HoldSeconds` exist in `tools/dev-snapshot.ps1`.

## Acceptance

- A table in `runs/286-animated-scenery-plays/README.md`: each animated kind, a map, moving in classic (yes or no), moving in 3D (yes or no), with the frame pairs.
- Every kind still in 3D but moving in the original is fixed by a rule, or pinned with its cause.
- ctest passes; perf-check before and after on any renderer change.

## Out of scope

Critter animations and combat effects.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
