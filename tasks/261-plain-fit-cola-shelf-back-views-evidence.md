---
id: 261
title: Verify plain-fit cola and shelf back textures
milestone: M1
status: todo
owner: any
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [125]
reviewed: no
---

## Goal

Verify that the opposite art slots used as back textures on plain-fit cola
machines and shelves are actual rear drawings rather than unrelated variants.

## Context

- Task 125's acceptance names a cola machine and shelf, but its Log says both
  shots failed framing: the cola was behind a fence and the shelf was too
  small at fly distance.
- The 125 coverage gate checks opaque-pixel coverage only. It cannot determine
  whether an opposite slot is semantically a rear view or a variant.
- Task 259 checks whether scenery turns with the fly camera. This task checks
  only the opposite slots used on solid back faces; do not change or duplicate
  the sprite-turn rule.

## Acceptance

- Capture a plain-fit cola machine and a shelf solid from a front control view
  and a matched rear fly quarter, before and after selecting the far face's
  source art. Frame each object large enough to read the art, with the far face
  unobstructed.
- For each object, identify its art file, placement/tile, near rotation and
  opposite slot. Compare the opposite slot to the game's original art and state
  whether it depicts the object's rear or a different variant/damage state.
- Confirm in the matched before/after frames that the chosen source matches the
  object from behind and has no see-through texels. If a shelf slot is a
  variant, record it as such and do not describe it as a rear drawing.
- Store matched JPEG pairs and JSON sidecars under
  `runs/261-plain-fit-cola-shelf-back-views/`. Record map, elevation, object,
  art, tile, camera turns, build commit and setting state. Keep each image
  under 1 MB.
- If either object cannot be framed or its art cannot be identified, record
  the exact attempt and blocker; do not claim that object verified.

## Out of scope

Sprite rotation behavior, changes to the back-sprite coverage threshold,
vehicle models, and changing the art assets.

## Log

### 2026-11-12 gpt review
Done: split the unverified cola and shelf back-texture evidence from task 259,
which covers scenery rotation exceptions. ID 261 is free after checking task
files, remote task branches and PRs through ID 299; PR #262 is repository URL
maintenance, not task 262.
Next: capture readable matched views and determine whether each opposite slot
is truly the object's rear art.
Check: not run; evidence task.
