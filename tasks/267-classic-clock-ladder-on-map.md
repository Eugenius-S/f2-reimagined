---
id: 267
title: The classic clock ladder captures the map it was asked for
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`tools/dev-snapshot.ps1 -ClockLadder -Classic -Map <map>` saves the original 2D picture of that map at each hour of the ladder.

## Context

Draft PR #472 adds the mode, but two runs captured ARTEMPLE instead of `city1`: the ladder starts before the requested map is loaded. Finish that branch. It unblocks 262 and 263, which need matched 2D day and night frames.

## Acceptance

- One run on `city1` and one on an `encspecial` map: every sidecar names the requested map and elevation, and the frames differ between 22:00 and 09:00.
- A map that never loads fails the run with a clear message instead of capturing another map.
- ctest passes; the 3D clock ladder still logs `CLOCK_LADDER PASS` on kladwtwn.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.

### 2026-11-13 gpt
Done: `-ClockLadder -Classic` waits for the requested map and elevation, reports an unregistered target clearly, and validates every sidecar plus the 22:00/09:00 image difference. Fixed Classic mode's extra map-tour capture and supplied the initial one-shot request for the existing 3D ladder. Compact frames and the evidence table are in `runs/267-classic-clock-ladder/`.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` passed; `ctest --test-dir build --output-on-failure` passed 27/27. `tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder` passed with four `CITY1.MAP` e0 sidecars; `-Map rndwhale` passed with four `RNDWHALE.MAP` e0 sidecars. `tools/image_diff.py` reported 1,627,640 and 1,664,908 changed pixels (>8) for 22:00 vs 09:00. `-Map map-does-not-exist` exited with `requested-map-not-in-registry` and no ladder capture. `-Map kladwtwn -ClockLadder` logged `CLOCK_LADDER PASS` in 3D. Local sidecars: `build/classic-clock-267-city1-final/` and `build/classic-clock-267-encspecial/`; invalid-map diagnostics: `build/classic-clock-267-invalid-map/process.stdout.log`.
Next: controller review and merge of draft PR #472. A performance baseline was not applicable: this only changes the developer snapshot path and map startup, with no renderer or map-load implementation changes.
