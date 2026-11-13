---
id: 277
title: The Shark Club marquee stands right in 3D
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

The Shark Club marquee on newr2 e0 looks in 3D as it does in the original.

## Context

- 224's find F3, left open twice. The find's line and frame are in `tasks/224-*.md` and `runs/224-geometry-second-pass/`.
- A large sign is one of 173's kinds: if the exporter cannot build it, it stands as a billboard or a fixed plane. Apply the rule to every sign of this kind in New Reno, not to one object.

## Acceptance

- Classic | 3D | turned +30 frames of the marquee and of two other large Reno signs in `runs/277-shark-club-marquee/`.
- ctest and the pixel regression pass; changed spots listed.

## Out of scope

A modelled 3D sign (229's reference views come first).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
