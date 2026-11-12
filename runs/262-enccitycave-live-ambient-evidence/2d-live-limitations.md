# Live 2D capture attempts

Task 262 requires original 2D day/night comparisons for a representative city, mountain and cave map. The 3D clock-ladder evidence is complete, but no live 2D frame was obtained. Two available routes were checked and neither can advance time in the classic view.

## Attempt 1: ClockLadder with Classic

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -ClockLadder -CaptureConsole -TimeoutSeconds 30 -SceneCache build/scene-cache -OutputDirectory build/262/classic-clock-ladder-city1
```

The helper rejects `-Classic` in the `-ClockLadder` parameter check (`tools/dev-snapshot.ps1:255`) before starting a game. PowerShell's script trap then reports `Release-TestSlot : The term 'Release-TestSlot' is not recognized`, hiding the original validation message. No classic capture ran.

## Attempt 2: Classic view with scripted Ctrl+T

A scene-input script tried `hold 224`, `key 23`, `release 224`, then `snap`, repeating the Ctrl+T sequence to capture 22:00, 02:00, 06:00 and 09:00. Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map city1 -Elevation 0 -Classic -SceneInput build/262/classic-clock-script.txt -DevInputShots 4 -CaptureConsole -TimeoutSeconds 20 -SceneCache build/scene-cache -OutputDirectory build/262/classic-sceneinput-city1
```

The background game stayed in view 0 for the full run. Its log had no `DEV_INPUT` line and no snapshot JSON/image. The helper stopped with `Scripted input did not complete with exactly 4 snapshots.` In `ce/src/devtools/devtools.cc:2315`, scene input starts only after `scene::activeView() == scene::View::Full`; the Ctrl+T handler is also gated by `scene::enabled()` at `ce/src/devtools/devtools.cc:4193`. The existing clock ladder is handled only from the 3D frame path (`ce/src/devtools/devtools.cc:3534`).

No tool or rendering code was changed for this evidence task. A dedicated live classic clock capture path is needed before the 2D comparison can be completed. Until then, do not infer a 3D-only defect from the 21 changed 3D elevations.
