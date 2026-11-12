# Classic clock ladder diagnostic

## Result

The harness reached `CLOCK_LADDER PASS mode=classic` and produced four original 2D framebuffer captures with real game times 22:00, 02:00, 06:00, and 09:00. Both attempts were on `ARTEMPLE.MAP`, not the requested `city1`; the first attempt's image visibly showed the opening movie. These captures do not count as matched map evidence for tasks 262 or 263.

## Attempts

1. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder -CaptureConsole -TimeoutSeconds 60 -SceneCache build/scene-cache -OutputDirectory build/classic-clock-test`
   - Ladder log reported four stages and PASS.
   - All four JSON sidecars in `build/classic-clock-test/` reported `ARTEMPLE.MAP`; `scr00000.png` visibly showed the opening movie.
2. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder -SkipScriptBlocking -CaptureConsole -TimeoutSeconds 90 -SceneCache build/scene-cache -OutputDirectory build/classic-clock-test2`
   - Same result: four stages and PASS, but all sidecars reported `ARTEMPLE.MAP`.

## Diagnosis and current code

The Classic ladder callback originally ran as soon as a map name and player existed. It could therefore capture the startup screen before the requested startup map finished applying. The current draft waits for `sceneStartupReady()` and for the exact requested map and elevation before setting time or capturing. That readiness gate has compiled but has not been exercised in a live capture; the two-attempt limit was reached before another run.

## Checks

- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`: passed.
- `ctest --test-dir build --output-on-failure`: 27/27 passed.
- `py -3 tools/check_public_content.py origin/main`: passed before the initial push; rerun before any further push.
- Not checked: live Classic capture after the readiness gate, matched day/night evidence on a requested map, image comparison, and `tools/perf-check.ps1` before/after.

PR #472 remains a draft. No changes were made to tasks 262/263 or their PRs.
