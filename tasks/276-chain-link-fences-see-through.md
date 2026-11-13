---
id: 276
title: Chain-link fences show what is behind them
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A chain-link fence in 3D is a mesh you see through, not a black wall with specks.

## Context

- The owner's find of 2026-11-08, 191 find 1: the Den's fences (denbus1) stand as solid black walls with bright specks. Crop: `runs/191-furniture-props/den-fence-black.jpg`.
- Fences have their own working algorithm (173's note): find why this art loses its transparency (the alpha of the mesh holes, the draw order, a shadow or see-through pass) before changing the algorithm.

## Acceptance

- Before | after crops on denbus1 and on every other map that uses this fence art (list them by art id), game camera and one turned view, in `runs/276-chain-link-fences-see-through/`.
- The ground and objects behind the fence are visible through the holes; the wire itself keeps its colour.
- ctest and the pixel regression pass; changed spots listed.

## Out of scope

Other fence kinds that already look right.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
