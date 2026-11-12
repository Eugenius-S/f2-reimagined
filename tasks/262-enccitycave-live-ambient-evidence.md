---
id: 262
title: Verify live ambient dimming on city cave and mountain encounters
milestone: M1
status: blocked
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

### 2026-11-12 gpt
Blocked: representative live 2D day/night comparisons could not be captured after two route checks. Preserved matched live 3D ClockLadder evidence for all 28 elevations in `runs/262-enccitycave-live-ambient-evidence/`, with four stages (22:00, 02:00, 06:00, 09:00), full metadata sidecars and JPEG sheets under 1 MB. The 22:00 to 09:00 pixel comparison of the shared upper 880 pixels shows visible changes in 21 elevations; cave1-e1, cave2-e1, cave3-e1, cave4-e1, cave5-e2, cave6-e0 and cave7-e0 remain unchanged. Ambient rises from 28426 at 22:00 to 65536 by 06:00 on the changed elevations. No 3D-only defect is claimed without matched 2D evidence.

Attempt 1: `tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder ...` was rejected by validation at line 255 before game start; its trap replaced the validation message with `Release-TestSlot ... not recognized`. Attempt 2: `-Classic -SceneInput` with a Ctrl+T script ran until the 20 second timeout with `view=0`, no `DEV_INPUT` lines and no snapshot, then reported `Scripted input did not complete with exactly 4 snapshots`. Scene input only starts when the active view is Full (`ce/src/devtools/devtools.cc:2315`); Ctrl+T requires `scene::enabled()` (`ce/src/devtools/devtools.cc:4193`); ClockLadder runs only on the 3D frame path (`ce/src/devtools/devtools.cc:3534`). Exact command/output details: `runs/262-enccitycave-live-ambient-evidence/2d-live-limitations.md`.

Check: Release build passed with `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`; `py -3 tools/scene_export/export.py --game-root E:/Projects/fallout2-ce-3d/game --maps enccitycave --cache-root build/scene-cache` reported 22 maps and 28 scenes; all 28 `tools/dev-snapshot.ps1 -ClockLadder` runs logged `CLOCK_LADDER PASS`; evidence assembly validated map/elevation, view tile, camera, renderer, resolution and settings across all four captures.

Not checked: representative live 2D day/night comparisons, original 2D lighting against the 3D change, and any 3D-only rendering defect. Next: a separate live classic clock capture path is needed to resume this task.