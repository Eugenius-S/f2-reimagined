---
id: 148
title: The README shows the camera turn and combat in 3D
milestone: M1
status: done
owner: glm
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [146]
reviewed: yes
---

## Goal

A player reading the README (and README.ru.md) learns that the camera turns
with `[` and `]` in 10-degree steps up to 30 (more with the unlimited
option) and that fights now play in 3D, each with one picture.

## Context

- Batch: the README (2026-11-04 inbox sweep). Folded in, the inbox line of
  2026-11-01: put the two night projectile clips from 097 (laser pistol,
  plasma rifle) side by side in one video, compressed for GitHub, in the
  middle of the README.
- 144 (#205, #207) added the turn steps; 135 step 1 (#211) keeps 3D on in
  combat. 105 wrote the README's feature list.
- Pictures: the 144 snapshots (`runs/144-camera-turn-steps/`) and one 146
  combat capture, compressed for GitHub, inside the repository's size rule.

## Acceptance

- Both READMEs carry the two features with a picture each, in the existing
  feature list's style; README.ru.md is the only Russian file.
- The runs-size check passes.

## Log

- 2026-11-01 Cut with the 146 pool.

### 2026-11-12 glm: both READMEs show the camera turn and 3D combat

Done: README.md and README.ru.md carry the two features with a picture
each, in the feature list's style:
- the camera turn: the bullet now names `[` and `]` in 10-degree steps up
  to 30, with the four-step sheet from 144's snapshots
  (docs/images/camera-turn-steps.jpg, 191 KB);
- 3D combat: a new bullet (the outlines, the hex cursor with its to-hit
  number, the attacks and the enemy turns in the 3D view) with 146's
  kladwtwn capture (docs/images/combat-3d.jpg, 243 KB);
- the stale line "During combat and dialogue the game shows the original
  view for now" replaced: dialogue keeps the original presentation, combat
  plays in 3D;
- the Keys section lists `[` and `]`.
README.ru.md is the only Russian file; both edited together. Checks:
tools/check_runs_size.py origin/main passes, check_public_content clean.
Not verified: a reader's look (user-check: yes); the 097 projectile clips
stay their own line (097 has no PR).
