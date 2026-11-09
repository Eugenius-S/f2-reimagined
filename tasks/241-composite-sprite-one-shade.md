---
id: 241
title: A building made of several sprite pieces takes one sun shade, not a patchwork
milestone: M1
status: todo
owner: claude
suggested: claude
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

A composite sprite (one building or rock face cut into several art pieces)
is shaded by the sun as one body. Owner rule, 2026-11-09: when the group
stands on a plane in shadow and one piece falls in the shadow, the whole
group is shaded.

## Context

- Owner find: the Temple of Trials entrance (artemple e0) is part in shade,
  part in sun. Shot `<Steam install>/shots/dev/2026-11-08/20261108-221627-artemple-e0.png`:
  the left dome has a lit rectangular band at its top and a lit strip in its
  middle while the rest is shaded.
- Reproduced on main 0a825d85a: `tools/dev-snapshot.ps1 -Map artemple
  -Tour arcaves:0 -Hour 16` with `[3d] shadows=1` and `sun=1` in the test
  config (the plain `-Map artemple` hero preview comes out black; the tour
  gives the real frame).
- The temple is eight scenery pieces, TEMPLE01..TEMPLE05, TEMPLE06,
  temple07, temple08 (scenery.lst indices 0x55d..0x567), on tiles 13085,
  13480, 14276, 14490, 14870, 15884, 16285 and 16491. Mountains are the same
  kind of set (mount01..06, MOUNTN01..02).
- Mechanism: a standing card taller than half a wall takes one whole verdict
  at its own feet (`feetShadow`, `ce/src/scene_renderer/live_objects.cc:391`;
  `sunShadowAtAnchor` at `ce/src/scene_renderer/scene_shader.cc:565` via
  `sunShadowCard` line 598). Each temple piece has its own feet, so one piece
  reads lit while the next reads shaded.

## Design notes (to confirm in the work)

- The group: pieces of one art set (same name stem, different numbers) whose
  cards touch or overlap on screen, with no art file repeated in the group.
  A forest of TREE7/8/9 repeats its files, so it never groups. Known gap: two
  different trees that overlap alone would group; mark it as a shortcut.
- The verdict: each piece passes the group's feet anchors (a small uniform
  array, cap it and mark the cap) and the shader takes the darkest one.
- Check first whether the pieces cast into the shadow map themselves. If they
  do, a piece's feet inside another piece's own shadow would darken the whole
  group at every hour, so the group's own casters must be excluded at the
  taps, the way `selfId` excludes a card's own caster today.
- Lower cards (stairs, braziers) read per pixel and stay as they are.

## Acceptance

- artemple at hours 9, 12 and 16: every TEMPLE piece shows the same verdict
  (all lit or all shaded), shots before and after in `runs/241-composite-sprite-one-shade/`.
- A forest edge (artemple's tree rows) keeps per-tree verdicts.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Per-pixel cast shadows across a building's face; the black sides of the
artemple entrance (inbox, grow the map with pines).

## Log

- 2026-11-09 claude: cut from the owner's find; mechanism and pieces found, reproduction above.
