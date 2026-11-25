---
id: 331
title: Ground lighting follows changes to the active map clip
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

At a fixed camera, the cached ground light stays correct when the effective map clipping changes.

## Context

Narrow correctness follow-up to merged PR #590, separate from task 328's FPS work and GLM's geometry re-walks. The floor bake shader clips fragments before writing. The cache key does not currently include the effective clip enable state or plane coefficients, so newly exposed terrain can sample the prior cleared light value until another input invalidates the bake. Reproduce against current main before calling this a visible gameplay defect.

## Acceptance

- A real map at fixed pose exercises a change to effective clipping without depending on a camera move. Record baseline and candidate results, including whether the fault is visible.
- The first frame after a change uses the correct bake, and a stable clip still reuses it. Account for changes to enabled planes as well as the on/off switch.
- The fix leaves map boundaries, cameras, interaction and the image with unchanged settings intact. Use a matched still comparison with sun shadows enabled.
