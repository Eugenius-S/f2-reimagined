---
id: 329
title: The red CLOSED ART placeholder cards never show in the 3D view
milestone: M1
status: done
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
- 2026-11-26 glm: took the task. First reproduction step: the 281 evidence sheet pairs a classic shot at tile 20985 with a 3D shot at tile 20695 (the walk's raw devshot sidecars in build/walk/281 name the different tiles). Re-shot both engines at each tile on the walk's own build and scene cache: tile 20695 shows the same red placeholder field in BOTH engines, tile 20985 is empty black in BOTH. The cards read UNUSED ART (shut058.frm, the floor art of 94 squares), not CLOSED ART; the 3D projection squashes the text. Verdict so far: no defect to fix; evidence and the placeholder sweep go into runs/329-closed-art-placeholders-stay-invisible/ before the verdict lands.
- 2026-11-26 glm: DONE, no code change. Evidence: runs/329-closed-art-placeholders-stay-invisible/ (01 the pad tile pair - same field in both engines, 02 the 20985 pair - empty in both, 03 the cowbomb control - 25 objects with missing BLACK001.frm draw nothing in both, 04 the shut058.frm UNUSED ART card; README has the tile table, the commands and the sweep). Sweep over all 176 maps: 23250 floor + 6166 roof squares have no missing tile art; five object art misses in the whole game (cowbomb BLACK001.frm x25, critter variants hmmaxx/hanpwr/nmmaxx, one out-of-range vault15 FID) and every one draws nothing in both engines, so the one-sided rule (2D empty means 3D empty) already holds everywhere. The red field is UNUSED ART floor art the 2D draws, so it stays visible in 3D (the 283 walk recorded the same match). Repro of the non-defect: tools/dev-snapshot.ps1 -Map sfshutl2 -Elevation 0 -ViewTile 20695 -Still (-Classic for the classic half) on the walk's build d6ad165260a22c7e with the walk's own build/scene-cache. No ctest, regression or tour needed: nothing but the task file and the run folder change. Next step: none - the find is settled; a real closed-art case, if one ever turns up, cuts a new task.
