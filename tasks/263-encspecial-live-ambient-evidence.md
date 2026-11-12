---
id: 263
title: Verify live ambient lighting and missing entries for special encounters
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Record what the actual play-time ambient transition looks like in 3D across
the special encounter elevations and close the three missing live map views
from task 171. This is an evidence task, not a claim that the maps have a
rendering defect.

## Context

- Task 171's 8/12/18 background `--dev-hour` sweep does not prove the normal
  play-time ambient change. Task 170 confirms background captures retain the
  default ambient at every `--dev-hour`; task 262 covers only city, cave and
  mountain encounters. Do not duplicate task 262's map set.
- Task 171's live entry probes cover the 25 spot elevations, but not encfite
  e1/e2: its enter script lands on e0. The empty-atlas encode/decode test
  checks the data format, not entering those elevations in game. Task 124
  records old export failures there but did not retest them; task 206 checks
  encfite e0 entry stability only.
- Task 171 has no cowbomb live view because its enter script teleports the
  test run to artemple. Task 169's registry-less desert1 workaround is a
  different case, and task 173 does not provide a way to remain on cowbomb.
- Use the repository's background test game and an isolated worktree cache.
  Do not run `tools/dev-launch.ps1`, close or focus the owner's game, or write
  to the owner's `game/mod-cache`.

## Acceptance

- Capture matched live 3D views at daytime and nighttime for all 25
  encspecial elevations that task 171 already enters. Keep map, elevation,
  tile, camera, resolution, renderer and settings matched. Record actual
  in-game time and ambient values from the capture metadata or log where
  available. Do not use `--dev-hour` alone as proof of play-time dimming.
- Include original 2D comparisons for representative outdoor and indoor
  encounters to distinguish expected map lighting from a 3D-only change.
- Find a background test route that enters encfite e1 and e2 and remains on
  each elevation long enough to capture it. Record the exact map and
  elevation in the capture metadata. If the route cannot be made to enter,
  record the attempts and limitation without treating encode/decode as live
  entry proof.
- Find a background test route that remains on cowbomb despite its enter
  script. Record the exact map in the capture metadata. If it cannot be held,
  record the attempts and limitation without counting an artemple capture.
- Save compact evidence and metadata under
  `runs/263-encspecial-live-ambient-evidence/`. Summarize which maps and
  elevations visibly change. If a reproducible 3D-only defect appears, file
  it separately with exact map, elevation, tile, time and before/after
  evidence. Do not change renderer or map data in this evidence task.

## Out of scope

Renderer or exporter changes, map-specific fixes, and claims that background
`--dev-hour` proves live ambient behavior.

## Log

- 2026-11-12, gpt review. Task 171 review found that its 75 background
  `--dev-hour` captures do not prove actual play-time dimming, and that live
  captures omit encfite e1/e2 and cowbomb for separate script-driven entry
  reasons. Task 262 covers city/cave/mountain encounters, not this map set.
  No graphical defect is claimed. Next: capture matched live day/night views
  for the existing 25 elevations and find or document test routes for the
  three missing map/elevation entries.

- 2026-11-13 claude (review of gpt's run): on landing, commit the comparison JPEGs and one table of map, elevation, hour and changed-pixel count; leave the raw devshot JSON sidecars out of `runs/` (the draft carries 16-18 thousand lines of them).
