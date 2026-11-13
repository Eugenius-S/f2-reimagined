---
id: 263
title: Verify live ambient lighting and missing entries for special encounters
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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
### 2026-11-12 gpt
Done: Captured matched live ClockLadder views for all 25 existing `encsp-*` spots at 22:00, 02:00, 06:00 and 09:00. All 100 sidecars match map, elevation, tile, camera, renderer and display settings. Added compact sheets and original 2D framebuffer samples for rndwhale e0 and encfite e0 under `runs/263-encspecial-live-ambient-evidence/`. No renderer, exporter or map data changed.
Next: Owner follow-up is needed for live entry evidence at encfite e1/e2 and cowbomb; background attempts reached e0 or artemple/cardesrt. See `route-attempts.md` for the two distinct attempts per target.
Check: `configure-build.ps1 Release`; `export.py --maps encspecial` reported 23 maps / 28 scenes; sidecar audit verified all 25 sets and every JPEG is under 1 MB. Each original sidecar records the derived `clock_ladder` hour, ticks and ambient for its captured game state.
Not checked: Matched timed original 2D day/night views. Task 262 records that the supported ClockLadder and SceneInput methods cannot run in Classic view; representative static 2D captures are included. No 3D-only defect is claimed.
### 2026-11-13 gpt
Done: Rebased the existing #463 branch onto `origin/main` at `e4529245e` and rebuilt Release. Revalidated the 25 earlier live 3D spot ladders at 22:00, 02:00, 06:00 and 09:00. Their 100 sidecars agree per map/elevation on tile, camera, renderer, resolution, settings and one historical executable SHA. Added matched current-build 3D and original 2D Classic ladders for outdoor `RNDWHALE.MAP e0` tile 17705 and dark indoor `ENCFITE.MAP e0` tile 20100. The one CSV records 108 rows and changed-pixel counts. All raw sidecars and PNGs were moved to ignored `build/task263-*`; runs contains seven compact contact JPEGs and no JSON. No renderer, exporter or map data changed.
Next: No further work in this evidence task. The two distinct prior routes per missing target remain documented as limitations.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`; `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map rndwhale -Elevation 0 -ViewTile 17705 -ClockLadder -SceneCache build/scene-cache -OutputDirectory build/task263-3d-current/rndwhale-e0`; repeat with `-Map encfite -ViewTile 20100`; repeat both with `-Classic -ClockLadder` and output under `build/task263-2d/`. A metadata audit confirmed all 25 historical 3D ladders and the 16 current matched ladder frames. Current executable SHA-256 is `c126ab6574c2c8c8227252689c0d2b6587463bf408b240808ef2f9ce42a7e908`; the historical 3D set used `58dc5c0ac3dc1d5cf522eb813196c23e9dbc6f5f696312e2d989a2187695526b`. Outdoor 3D changed 1,618,237 pixels and 2D changed 1,655,008 from 22:00 to 09:00; indoor changed 0 in both modes. The seven contact sheets are below 1 MB each. The near-white scan found at most 14 near-white pixels in a 3D world panel. `capture-index.csv` references local sidecars such as `build/task263-sidecars/rndwhale-e0/devshot-20261112-110612-000.json` and current ones under `build/task263-2d/` and `build/task263-3d-current/`.
Not checked: Exact KLAMALL tile 28096 reproduction from Task 270 is separate and was not rerun. The direct and map-tour routes for `encfite e1/e2` and cowbomb failed as logged; no live entry is claimed. No reproducible 3D-only defect was found.
