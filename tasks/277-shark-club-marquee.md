---
id: 277
title: The Shark Club marquee stands right in 3D
milestone: M1
status: doing
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
- 2026-11-19 glm: the defect reproduced cleanly (devshot -Map newr2 -ViewTile
  20100, the frame saved to the branch's runs folder on push). The marquee's
  art found: casent26.frm (178x473, the wide lit sign board) with
  casent25.frm beside it, scenery at e0. Mechanism identified: the roof cut
  (task 114) - the scene shader discards a card's part ABOVE the roof plane
  wherever the roof grid has a roof square at that world square
  (scene_shader.cc, the uRoofClip texelFetch branch). The marquee's card
  crosses the building's roof squares' hexes, so its upper half discards -
  the black band under the roof texture. The vanilla 2D draws the sign's
  full art over the roofs (objects draw after tiles), so the original shows
  it lit. The fix direction: the roof cut must distinguish what is BEHIND
  the roof plane (hide) from what stands IN FRONT of it at the same squares
  (the marquee - keep); that is roof/wall-hole class surgery (190's
  territory), not a one-line change - the next session or 190's owner.
  Evidence: the fresh frame rides with this branch's push (recompressed to
  JPEG after the runs-size 1 MB cap flagged the PNG).
