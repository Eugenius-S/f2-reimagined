---
id: 224
title: Geometry second pass: re-walk the 3D-live maps hunting and fixing flaws
milestone: M1
status: todo
owner: glm
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A second pass over the geometry of every map that is live in 3D: walk
the spot list with fresh eyes, find what is wrong in the rendered
scene (a thing sunk, floating, poking through, split, z-fighting,
missing, an edge gap), and fix each find or hand it to its batch.

## Context

- Folded in from 243 (2026-11-09, owner: one object is not a task of its own): the artemple entrance base (stairs, dome bottoms) is baked into the floor and shades as ground while the building cards above take one shade (241). Look for the same on every multi-piece building in the pass; the exporter should leave such a base to the cards. Details in `tasks/243-composite-building-not-baked-into-floor.md`.
- The owner's ask (2026-11-06): a second geometry pass, hunting flaws
  and fixing them.
- Earlier sweeps each covered one slice: 075 the shapes algorithm, 129
  the Klamath graphics sweep, 168/170/171 the map batches, 190/191/192
  the roofs/furniture/vehicles batches. This pass re-walks the whole
  live set with the tooling those landed: the spot list in
  tools/regression/spots.json (154 spots, 111 maps), the owner shot's
  region mark (183) for pinning a find to hexes and objects, the
  teleport for reaching any spot in a live run.
- A find is pinned by spot, hex and frame before any fix: no
  silhouette-metric hunts (the beds' lean is the art, not a bug), and
  a proto's material line lies (the "Wooden Table" that is metal).

## Acceptance

- Every spot of the tour list reviewed; the pass's frames and the
  find register in runs/204-geometry-second-pass/ (one line per find:
  spot, hex, what is wrong, frame).
- Each find either fixed (its own PR, the gate green on the touched
  spots' stills) or folded into its batch task (190/191/192) or the
  inbox with the evidence link.
- No reference refresh without the owner's word.

## Out of scope

The floor extension pass (202), the regression harness (203), new
maps entering 3D (187), view-persistence (173, the owner's draft).

## Log
- 2026-11-08 claude: owner order 2026-11-08: 240 (inner-wall graffiti over the outer wall, kladwtwn tiles 28890/29091/29291, crop `runs/240-decal-through-wall/`) folded in; this pass waits behind glm's 239, 172, 211, 212.
- 2026-11-10 glm: the agent-side precursor sweep of the owner's city
  review (175–182) done - 66 city map-elevations × (hour12 3D,
  hour18 3D, classic 2D) on exact main ba79b58d; 183/198 frames
  valid and machine-validated, all judged. Three finds, all
  exporter/renderer-level, each wanting its own diagnosis:
  F1 kladwtwn e0 black map-edge wedge (2D dirt road continues
  there), F2 denbus2 e1 whole-elevation geometry missing (only a
  dirt strip + corner patch render; 2D shows the full level),
  F3 newr2 e0 Shark Club marquee upper sign face black (2D fully
  lit). Evidence pairs in `runs/224-geometry-second-pass/finds/`,
  register + coverage in `runs/224-geometry-second-pass/README.md`.
  Five maps hang the engine at load (classic too): klacanyn e0,
  klatoxcv e0/e1, broken2 e1, newrvb e0 - own task 246; their
  sweep coverage waits on it. Non-finds and the one low-confidence
  leftover (artemple e0 right-edge boulders) in the README.
  The full ~420 MB shot set stays on the machine, not committed.
