# Task 263 live ambient evidence

## Capture method

Built `Release` on rebased `origin/main` with `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`. The executable used for current matched captures has SHA-256 `c126ab6574c2c8c8227252689c0d2b6587463bf408b240808ef2f9ce42a7e908` and reports CE commit `263c1060c` in each sidecar.

The original 25 live 3D spot ladders cover all `encsp-*` regression spots from `tools/regression/spots.json`: four actual game times, 22:00, 02:00, 06:00 and 09:00. Their 100 sidecars use one consistent earlier executable SHA-256, `58dc5c0ac3dc1d5cf522eb813196c23e9dbc6f5f696312e2d989a2187695526b`. They remain comparable within each ladder. The 23 other spot ladders were not compared against a 2D capture from a different build.

After #267 merged, `-Classic -ClockLadder` provided a new live 2D route. To compare across modes on one executable, I captured fresh 3D and original 2D ladders on this Release for outdoor `RNDWHALE.MAP e0`, tile 17705, and indoor `ENCFITE.MAP e0`, tile 20100. All eight captures per map share map, elevation, tile, in-game time, 1920x1080 drawable, 960x540 logical size, scale 2, `scene-gl`, and executable SHA. In 2D the metadata says `view: vanilla-framebuffer` and `scene_view: off`; in 3D it says `scene_view: full`. The 3D camera is the fixed scene camera centered on the same view tile; the 2D camera is the original view centered on that tile.

`capture-index.csv` is the only table. It has 100 3D rows for all 25 spots and 8 new 2D rows for these representative pairs. Each row records map, elevation, tile, actual game time and ambient, camera and view mode, renderer and display settings, CE commit, executable hash, contact sheet, `map_panel` and `stage_panel` indices, changed-pixel count from 22:00, pixel source, and local sidecar path. Counts compare pixels whose maximum RGB channel difference is greater than 8. The 23 historical 3D ladders use individual JPEG panels retained under ignored build; the four current mode/map ladders use original PNG frames before JPEG composition. Raw sidecars and PNG captures stay under this worktree's ignored `build/task263-*` folders.

## Observed changes

- Outdoor `RNDWHALE.MAP e0`: ambient is 28426 at 22:00 and 02:00, then 65536 at 06:00 and 09:00. Both current 3D and original 2D visibly brighten. Changed pixels from 22:00 to 09:00: 1,618,237 in 3D and 1,655,008 in 2D.
- Indoor `ENCFITE.MAP e0`: ambient stays at 55705 at all four times. The original 2D view is pixel-identical from 22:00 to 09:00, with 0 changed pixels. The current 3D view is also unchanged across the ladder. This is a stationary dark interior example.
- The earlier 3D set records 16 visibly brightening views and nine with no visible ambient change as listed by map in `capture-index.csv`. No 3D-only defect is claimed.

All 25 3D spot ladders were checked for blank white ground. A near-white scan (all channels at least 248) found at most 14 pixels in the 3D world portion of a panel, at `ENCDET.MAP e0` 22:00; manual inspection of the refreshed outdoor sheet shows textured ground in every stage. This verifies the Task 263 captures only, not the separate KLAMALL reproduction in Task 270. The 23 historical 3D spot ladders are grouped four map/elevations per contact sheet across six JPEGs. The matched current 3D and Classic ladders share one seventh sheet. Every page labels map, elevation, view mode and all four hours; each is below 1 MB.

## Missing live entries

No additional route attempts were made because the two distinct prior methods per target had already failed. Their outcomes and original metadata remain recorded locally under `build/task263-sidecars/runs/263-encspecial-live-ambient-evidence/route-attempts/`.

- `encfite e1`: direct startup reached `ENCFITE.MAP e0`; map-tour entry also reached `ENCFITE.MAP e0`.
- `encfite e2`: direct startup and map-tour entry both reached `ENCFITE.MAP e0`.
- `cowbomb e0`: direct startup was rerouted to `ARTEMPLE.MAP e0` and hit the 30-second watchdog; map-tour from `cardesrt` remained on `CARDESRT.MAP e0`.

These are documented limitations, not live entry proof. No encode/decode result or artemple capture is counted as cowbomb.

No renderer, exporter or map-data changes were made.
