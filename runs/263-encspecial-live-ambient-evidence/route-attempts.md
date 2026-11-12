# Task 263 route attempts

All attempts used the fresh Release executable, the worktree-local `build/scene-cache`, and the background `dev-snapshot.ps1` harness. Sidecars in each folder record the actual map/elevation reached.

| Target | Attempt | Command route | Actual result |
|---|---:|---|---|
| encfite e1 | 1 | `-Map encfite -Elevation 1 -ViewTile 20100 -ClockLadder` | Four captures report `ENCFITE.MAP e0`. |
| encfite e1 | 2 | `-Map encfite -Elevation 0 -Tour 'encfite:1:20100'` | Both captures report `ENCFITE.MAP e0`. |
| encfite e2 | 1 | `-Map encfite -Elevation 2 -ViewTile 20100 -ClockLadder` | Four captures report `ENCFITE.MAP e0`. |
| encfite e2 | 2 | `-Map encfite -Elevation 0 -Tour 'encfite:2:20100'` | Both captures report `ENCFITE.MAP e0`. |
| cowbomb e0 | 1 | `-Map cowbomb -Elevation 0 -ViewTile 20100 -ClockLadder` | Enter script moved to `ARTEMPLE.MAP e0`; watchdog stopped after 30 seconds with no image. See [watchdog diagnostic](cowbomb-direct-watchdog.txt). |
| cowbomb e0 | 2 | `-Map cardesrt -Elevation 0 -Tour 'cowbomb:0:20100'` | Both captures report `CARDESRT.MAP e0`; route did not enter cowbomb. |

No further route attempts were made. The route folders preserve the JSON metadata from each unsuccessful entry; `cowbomb-direct-watchdog.txt` records the actual map at the watchdog stop. Encode/decode or exported scene data is not treated as live entry proof.
