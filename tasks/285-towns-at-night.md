---
id: 285
title: Every town at night: lamps and fires light the street, nothing is pitch black or full bright
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

At 22:00 each town in 3D reads like the original at the same hour: lit windows, lamps and fires light their surroundings, dark corners stay dark but readable.

## Context

- 262 and 263 check the ambient level on encounter maps; no task sweeps the towns at night.
- `tools/dev-snapshot.ps1 -Hour 22`, with `-Classic` for the original side. The classic clock ladder is 267's work: if it is not merged, capture the classic side per map by hand and say so.

## Acceptance

- Classic | 3D pairs at 22:00 for one outdoor spot of every town map in `runs/285-towns-at-night/`, with the mean brightness of both sides in the README.
- Each pair whose brightness differs by more than a third, or where a light source the original shows lit is dark, is a find: fixed by a rule, or pinned with its cause.
- ctest and the pixel regression pass.

## Out of scope

Encounter maps (262, 263) and the glow's look (022).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
