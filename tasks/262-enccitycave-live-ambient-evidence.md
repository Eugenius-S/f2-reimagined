---
id: 262
title: Verify live ambient dimming on city cave and mountain encounters
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [170, 194]
reviewed: no
---

## Goal

Record whether the real play-time ambient transition is visible and consistent in 3D on every city, cave and mountain encounter elevation from task 170.

## Context

- Task 170 recorded an 8/12/18 background `--dev-hour` sweep, but explicitly notes that background captures retain the map's default light and do not exercise play-time ambient dimming. Task 194 verifies a live noon/dusk/midnight comparison only on one Klamath spot.
- This is an evidence task, not a claim that encounter maps currently have a rendering defect. Do not change renderer or map data based only on a visual difference; log any concrete defect separately for the applicable general fix batch.
- The target inventory is the 22 maps and 28 elevations listed in task 170. Preserve live capture evidence and metadata under `runs/262-enccitycave-live-ambient-evidence/`.

## Acceptance

- Use a real background game session with normal time progression or a verified in-game clock advance; do not use `--dev-hour` as proof of play-time ambient dimming.
- Capture matched 3D views at daytime and nighttime for all 28 elevations, with the same map, tile, camera, resolution, renderer and relevant settings. Record actual in-game time and ambient values from each capture's metadata/log where available.
- Include matched original 2D comparisons for representative outdoor city and mountain maps and a cave, to distinguish expected map lighting from a 3D-only change.
- Summarize which elevations visibly change, attach the evidence sheets and sidecars, and record any capture limitation. If a reproducible 3D-only defect is found, create or update the applicable general rendering task with exact map/elevation/tile/time and before/after evidence; do not fix it in this evidence task.

## Out of scope

Renderer changes, exporter changes, map-specific visual fixes, and claims that the background `--dev-hour` sweep proves live ambient behaviour.

## Log

- 2026-11-12, gpt. Source: review of task 170 / PR #291 found its background --dev-hour sweep does not exercise play-time ambient dimming; task 194 covers only one Klamath spot. No new live captures were taken for this follow-up. Next: capture matched live day/night 3D views for all 28 encounter elevations and representative 2D comparisons as specified above.

- 2026-11-13 claude (review of gpt's run): on landing, commit the comparison JPEGs and one table of map, elevation, hour and changed-pixel count; leave the raw devshot JSON sidecars out of `runs/` (the draft carries 16-18 thousand lines of them).
