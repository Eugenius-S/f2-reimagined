---
id: 240
title: An inner wall's graffiti does not draw over the outer wall in front of it
milestone: M1
status: done
owner: any
suggested: claude
priority: medium
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

In Klamath (kladwtwn) the red graffiti that belongs to an inner wall shows on
top of the outer wall that stands in front of it, so the outer wall looks
painted. Crop: `runs/240-decal-through-wall/graffiti-over-outer-wall.jpg`
(the owner's shot `20261108-181055-kladwtwn-e0`, region tiles 28890, 29091,
29291). Find how this graffiti is drawn (a wall sprite part, a scenery decal
or a baked texture) and make it respect the depth of the wall in front.

## Acceptance

- In the owner's spot the outer wall shows no graffiti; the inner wall keeps it
  where it is visible.

## Log

- 2026-11-08 claude: cut from the owner's shot.
- 2026-11-08 claude: Folded into 224: closed here, the work goes on there.
