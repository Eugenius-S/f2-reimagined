---
id: 269
title: The snapshot sidecar says which shadows, clouds, sun and glow were really on
milestone: M1
status: todo
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: [270]
reviewed: no
---

## Goal

Every devshot JSON records the effective state of sun shadows, cloud shadows, point lights, glow and ambient at the moment of the capture, read from the renderer, not from the command line.

## Context

- 2026-11-15: the implementation and blocked runtime checks live in draft PR #503. Finish with 270's valid floor capture after 306; continue that PR rather than starting a duplicate sidecar implementation.

143 (soft cloud edges) is blocked because six captures could not prove which toggles were in effect (PR #470). The same doubt sits in 256, where the 3D frame came out with white ground.

## Acceptance

- The sidecar has one object with the five values; a self-check run with `-CloudShadows` off and on shows the field change and the picture change together.
- A unit or script test reads a sidecar and fails when the field is missing.
- ctest passes.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.
