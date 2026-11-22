---
id: 277
title: The Shark Club marquee stands right in 3D
milestone: M1
status: done
owner: gpt
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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

### 2026-11-23 gpt

Reproduced the Shark Club roof-grid cut in native Game 3D and found the
same defect on Desperado. Matched renders identify just CASENT06 and
CASENT26 as the exterior canopies; adjacent walls keep their normal
clip. Verified Classic, Game 3D and a +30-degree turn at both places
and at the independent New Reno arch. The arch and boxing controls
passed the standard still pixel comparison with zero differences.
The standard New Reno fly tour covered eight spots at three turns
and all changes were localized to the restored Desperado canopy.

[Evidence and exact Windows comparisons](../runs/277-shark-club-marquee/README.md).
