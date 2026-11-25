---
id: 329
title: The red CLOSED ART placeholder cards never show in the 3D view
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

Objects whose art is the red CLOSED ART placeholder (or is otherwise missing) do not draw in the 3D view, matching the original 2D, which draws nothing for them.

## Context

- Found in the 281 re-walk (2026-11-25): SFSHUTL2 e0 around tile 20985 draws a field of about a hundred red "CLOSED ART!" hex cards lying flat over the black floor in the game camera; the classic 2D at the same tile shows an empty black picture. Evidence: `runs/281-rewalk-ncr-vaults-sf-enclave/sfshutl2-closed-art.jpg`.
- The red diamond with the CLOSED ART text is a placeholder inside the art data, not a renderer string. The original 2D skips these objects entirely, so the rule is: where the 2D picture shows nothing for an object, the 3D scene shows nothing either. The flat-picture rule ("an object that cannot be built in 3D yet stands as a flat picture") applies only to objects the 2D actually draws.
- Repro: `tools/dev-snapshot.ps1 -Map sfshutl2 -Elevation 0 -ViewTile 20985 -Still` against the classic run of the same spot; sweep other maps for the same placeholder once the rule lands (the shapes inventory or a scene-cache scan can list every object whose art resolves to the placeholder).

## Acceptance

- One rule, tested on SFSHUTL2 e0 t20985 (3D matches the classic black view) and on at least two more maps that carry the placeholder, showing the cards gone there too.
- ctest and the pixel regression pass; changed spots listed; the fly tour against main shows no other unintended changes.

## Out of scope

Objects with art the 2D does draw (their class fixes live in 189/191).

## Log

- 2026-11-25 glm: cut from the 281 re-walk find (see the 281 Log).
