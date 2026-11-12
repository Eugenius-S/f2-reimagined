# Task 262 live ambient evidence

Captured all 28 elevations in the background test game with the existing `ClockLadder` fixture. Each run recorded 22:00, 02:00, 06:00 and 09:00. It begins at 22:00, then advances the game clock through the Ctrl+T handler 11 times; each advance updates map scripts. No `--dev-hour` or `--dev-ambient` argument was used. Every sidecar confirms scene-gl, 1920x1080 output, and identical map/elevation, view tile, camera and settings across the four captures.

Overview: [all 28 matched night/day pairs](overview-28-elevations.jpg). Each elevation also has a named `map-eN.jpg` four-stage sheet and a matching `.json` metadata sidecar.

## Observed 3D changes

Pixel comparison uses the shared upper 880 pixels to exclude the HUD region. Visible changes between 22:00 and 09:00: cave0-e0, cave1-e0, cave2-e0, cave3-e0, cave4-e0, cave5-e0, cave5-e1, city1-e0, city2-e0, city3-e0, city4-e0, city5-e0, city6-e0, city7-e0, city8-e0, mountn1-e0, mountn2-e0, mountn3-e0, mountn4-e0, mountn5-e0, mountn6-e0.

No pixel change: cave1-e1, cave2-e1, cave3-e1, cave4-e1, cave5-e2, cave6-e0, cave7-e0. These seven cave elevations retain ambient 28426 throughout; the other elevations change from ambient 28426 at night to 65536 by 06:00. The observations alone do not establish a 3D-only defect because matched live 2D day/night captures could not be obtained. No rendering defect is claimed.

## 2D capture limitation

Attempt 1: `-Classic -ClockLadder` is rejected by `tools/dev-snapshot.ps1` validation before the game starts. Attempt 2: `-Classic -SceneInput` ran with view 0, but scripted input never started and produced no captures before timeout; the script pump starts only when the active view is Full. The current ClockLadder advances only on the 3D frame path. See [2d-live-limitations.md](2d-live-limitations.md). Task remains blocked on representative live 2D day/night comparisons.
