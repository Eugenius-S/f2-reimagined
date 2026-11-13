---
id: 262
title: Verify live ambient dimming on city cave and mountain encounters
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [170, 194]
reviewed: yes
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

- 2026-11-13 claude: commit only JPEG sheets under 1 MB and one map/elevation/hour/pixel-diff table in `runs/`; raw devshot JSON remains local.

### 2026-11-13 gpt
Done: Captured fresh matched 3D ClockLadder frames for all 28 elevations at 22:00, 02:00, 06:00 and 09:00. All 28 runs passed; sidecars match map, elevation, view tile, camera, output resolution, scene-gl renderer and settings within each four-frame set. Added representative original 2D ClockLadder comparisons for city1-e0, mountn1-e0 and cave0-e0 at matching map, elevation, view tile, scale, game time and ambient. Evidence is in `runs/262-enccitycave-live-ambient-evidence/`: two JPEG sheets under 1 MB and one 124-row pixel-diff CSV; raw frames and sidecars stay under local `build/262/`.

Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` passed. Fresh city1-e0 3D and all three 2D runs passed `CLOCK_LADDER PASS`; the remaining 27 3D runs also passed. The pixel table records night-relative changed pixels in the upper 880 pixels. 21/28 elevations visibly change; seven cave elevations remain unchanged and retain ambient 28426. The representative 2D frames show the same ambient transition. Visual review of the overview and full-size city1-e0, cave0-e0, mountn1-e0 frames found their floors textured, with no white-ground output or 3D-only defect.

Limit: Task 270's separate KLAMALL bake-enabled capture visibly has white ground (1,294,230 near-white pixels, 62.4%); its `F2CE_NO_BAKE=1` diagnostic shows textured ground. It did not reproduce in these task 262 target captures, so no affected frame was accepted as valid. Renderer diagnosis remains in task 270, outside this evidence task.
