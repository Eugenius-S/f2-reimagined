# Task 263 live ambient evidence

## Capture method

Built `Release` with `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`. Exported 23 `encspecial` maps / 28 scenes from the main checkout's game folder into this worktree's isolated `build/scene-cache`.

Captured the exact 25 `encsp-*` regression spots from `tools/regression/spots.json`. Each ClockLadder run used `tools/dev-snapshot.ps1 -Map <map> -Elevation <elevation> -ViewTile <tile> -ClockLadder -SceneCache build/scene-cache` and recorded four in-game time stages: 22:00, 02:00, 06:00 and 09:00. The ladder advances the real game clock through Ctrl+T. No `--dev-hour` or `--dev-ambient` was used. Every original JSON sidecar is retained next to its map sheet and records its `clock_ladder` hour, ticks and ambient. It points to the composite with `evidence_sheet` and `evidence_panel`. A [CSV index](capture-index.csv) lists all 100 samples. Raw PNG captures were removed after making the JPEG sheets.

All 100 sidecars were checked for map, elevation, view tile, camera, renderer, 1920x1080 resolution, 960x540 logical resolution, scale 2, and four actual game-time values. Every capture reports `scene-gl` and 3D enabled. Each JPEG is under 1 MB.

## Observed 3D ambient changes

Sixteen views visibly brighten from 22:00 to 09:00 as ambient rises from 28426 to 65536: [cardesrt e0](cardesrt-e0/cardesrt-e0.jpg), [encdock e0](encdock-e0/encdock-e0.jpg), [rndbess e0](rndbess-e0/rndbess-e0.jpg), [rndbhead e0](rndbhead-e0/rndbhead-e0.jpg), [rndbridg e0](rndbridg-e0/rndbridg-e0.jpg), [rndcafe e0](rndcafe-e0/rndcafe-e0.jpg), [rndexcow e0](rndexcow-e0/rndexcow-e0.jpg), [rndforvr e0](rndforvr-e0/rndforvr-e0.jpg), [rndholy1 e0](rndholy1-e0/rndholy1-e0.jpg), [rndholy2 e0](rndholy2-e0/rndholy2-e0.jpg), [rndparih e0](rndparih-e0/rndparih-e0.jpg), [rndshutl e0](rndshutl-e0/rndshutl-e0.jpg), [rndtinwd e0](rndtinwd-e0/rndtinwd-e0.jpg), [rndtoxic e0](rndtoxic-e0/rndtoxic-e0.jpg), [rnduvilg e0](rnduvilg-e0/rnduvilg-e0.jpg), and [rndwhale e0](rndwhale-e0/rndwhale-e0.jpg).

Nine views show no visible ambient change:

- The six Enclave interiors [encdet e0](encdet-e0/encdet-e0.jpg), [encfite e0](encfite-e0/encfite-e0.jpg), [encgd e0](encgd-e0/encgd-e0.jpg), [encpres e0](encpres-e0/encpres-e0.jpg), [encrctr e0](encrctr-e0/encrctr-e0.jpg), and [enctrp e0](enctrp-e0/enctrp-e0.jpg) keep ambient 55705 throughout. `encdock e0` is the exception and brightens.
- `rndbess e1` ambient changes from 28426 to 65536, but the elevation is an all-void black view and remains visually black: [sheet](rndbess-e1/rndbess-e1.jpg).
- `rndforvr e1` stays at ambient 65536 throughout: [sheet](rndforvr-e1/rndforvr-e1.jpg).
- `rndholy2 e1` stays at ambient 28426 throughout: [sheet](rndholy2-e1/rndholy2-e1.jpg).

Each of the other six unchanged Enclave views has a four-stage sheet and four verified metadata sidecars in its named directory. The observations do not establish a 3D-only defect.

## Original 2D comparisons

Captured original vanilla-framebuffer snapshots at the same map/elevation/view tile for [outdoor rndwhale e0](2d-live/rndwhale-e0/rndwhale-e0.jpg) and [indoor encfite e0](2d-live/encfite-e0/encfite-e0.jpg). Their JSON metadata confirms `view: vanilla-framebuffer` and `scene_view: off`. These are representative 2D map-lighting comparisons, not matched timed 2D ladders.

Task 262 already tried and documented two unsuccessful timed 2D methods: `-Classic -ClockLadder` is rejected before launch, and `-Classic -SceneInput` did not start input because the script pump requires the Full view. I did not repeat those attempts. The two Classic snapshots here use a separate supported capture mode and do not claim 2D day/night timing.

## Missing live map entries

At most two routes were tried for each missing map/elevation. Exact commands and actual sidecar map/elevation results are in [route-attempts.md](route-attempts.md). Neither target was captured:

- `encfite e1` and `e2`: direct startup and a map-tour entry each returned to `ENCFITE.MAP e0`.
- `cowbomb e0`: direct startup was sent to `ARTEMPLE.MAP e0` and hit the 30-second watchdog without a snapshot image. A second route from `cardesrt` via map-tour remained on `CARDESRT.MAP e0`. No `artemple` capture is counted as cowbomb.

No renderer, exporter or map-data changes were made. No reproducible 3D-only defect was found.
