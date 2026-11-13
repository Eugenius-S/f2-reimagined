# Classic clock ladder diagnostic

This file records the two original failed attempts and the later resolution. The readiness gate and the corrected harness have since been exercised live; see task 267 and `runs/267-classic-clock-ladder/evidence.md`.

## Result

The harness reached `CLOCK_LADDER PASS mode=classic` and produced four original 2D framebuffer captures with real game times 22:00, 02:00, 06:00, and 09:00. Both attempts were on `ARTEMPLE.MAP`, not the requested `city1`; the first attempt's image visibly showed the opening movie. These captures do not count as matched map evidence for tasks 262 or 263.

## Attempts

1. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder -CaptureConsole -TimeoutSeconds 60 -SceneCache build/scene-cache -OutputDirectory build/classic-clock-test`
   - Ladder log reported four stages and PASS.
   - All four JSON sidecars in `build/classic-clock-test/` reported `ARTEMPLE.MAP`; `scr00000.png` visibly showed the opening movie.
2. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder -SkipScriptBlocking -CaptureConsole -TimeoutSeconds 90 -SceneCache build/scene-cache -OutputDirectory build/classic-clock-test2`
   - Same result: four stages and PASS, but all sidecars reported `ARTEMPLE.MAP`.

## Diagnosis and current code

The Classic ladder callback originally ran as soon as a map name and player existed. It could therefore capture the startup screen before the requested startup map finished applying. The follow-up gate waits for `sceneStartupReady()` and for the exact requested map and elevation before setting time or capturing. Live runs now pass on `CITY1.MAP` and `RNDWHALE.MAP`, and a nonexistent map exits with `requested-map-not-in-registry`. The Classic harness no longer schedules an extra ordinary map-tour capture. The 3D ladder also receives its initial one-shot snapshot request.

## Checks

- Original attempts: Release build and `ctest --test-dir build --output-on-failure` (27/27) passed, but both captures were on `ARTEMPLE.MAP` and did not count as requested-map evidence.
- Follow-up task 267: Release and ctest passed; CITY1 and RNDWHALE Classic captures each produced four map/elevation-matched sidecars; 22:00/09:00 changed-pixel counts were 1,627,640 and 1,664,908 respectively.
- `-Map map-does-not-exist` exited clearly without accepting a ladder frame. The 3D `kladwtwn` ladder logged `CLOCK_LADDER PASS` after exporting its worktree scene cache.
- No performance baseline: the work changes only the developer snapshot path and startup checks, not renderer or map-load implementation.

PR #472 remains a draft for controller review. Tasks 262/263 and their PRs were not changed.
