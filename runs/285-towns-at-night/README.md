# 285: every town at night

Classic | 3D at 22:00 for one outdoor spot (each map's spawn tile) of every town
map from the city areas of city.txt: 68 maps. Mean brightness per side, measured
without the interface strip; every pair's sidecar game_time checked.

## Method

- `tools/dev-snapshot.ps1 -Map <m> -Elevation 0 -ViewTile <spawn> -Hour 22`
  and the same with `-Classic`; the classic clock ladder is 267's, merged.
- Brightness = the mean of the map window only (the classic framebuffer keeps
  the interface below row 416 logical; the 3D captures carry no HUD).
- First pass numbers (with the strip on the classic side) stay in
  `sweep-before-the-fix.csv`; the honest table is `sweep-hudfree-22.csv`.

## The review round

- ncr4's classic clock resets to 00:00 on every attempt (sidecar 792000 ->
  864003 twice). Its pair compares midnight on both sides below; the reset is
  recorded here instead of a 22:00 claim.
- Every other pair sits at 22:00 (ticks 792000..792003) on both sides,
  cardesrt, raiders1 and sfchina2 included.

## Find A, fixed by a rule: dark open-sky maps lost their night

The ground heuristic (commonest floor tile darker than 40 and no roofs ->
underground) called dark deserts underground, and an underground scene never
dims: at 22:00 cardesrt was 2.52x and raiders1 2.47x the classic's, and ncr4
was 2.75x at midnight
brightness. The map script's own `set_light_level` literals are the engine's
proof of the sky: a day level (>= 70) together with a night level (<= 60)
keeps the map outdoor whatever the ground looks like.

Change: vendor `fp/scene.py` (`build_ground`, `script_light_levels` extracted),
exporter 1.13.0 -> 1.14.0, 6 unit tests in `tools/scene_export/test_ground_sky_rule.py`
(the tests bind the real `script_light_levels`, so the byte-code reader runs for
real, including the missing-script-file case).
Flip set: 37 map elevations by the probe (dark deserts, coasts, mountains,
random encounters; not one cave or vault); the trial export confirms
ncr4/cardesrt/raiders1 and adds raiders1 e0, sfchina2 e1/e2.

| map | classic | 3D before | 3D after | ratio before -> after |
|---|---|---|---|---|
| cardesrt, e0 t20100, 22:00 | 34.41 | 86.57 | 50.02 | 2.52 -> 1.45 |
| raiders1, e0 t23305, 22:00 | 30.13 | 74.47 | 41.89 | 2.47 -> 1.39 |
| ncr4, e0 t17699, 00:00 | 24.57 | 67.57 | 37.53 | 2.75 -> 1.53 |

All three before shots and both after shots sit at the named map, hour and
tile. ncr4's midnight is measured, not assumed: the old-cache 3D at 00:00
draws 67.57 (the scene was underground and never dimmed at any hour), the
classic 24.57 comes through the 22:00 clock ladder - the script's night
`set_light_level` is a state the ladder sets and midnight keeps. A direct
00:00 start never runs that trigger and stays full bright (64.48), so the
ladder path is the faithful midnight.

Controls hold byte-stable: klatoxcv (pinned interior) 17.52 = 17.52, kladwtwn
(street) 29.38 = 29.38; their scene flags do not move. Sheets:
`cardesrt-fix.jpg`, `raiders1-fix.jpg`, `ncr4-fix.jpg`; the sweep pairs for
every find below sit next to them.

## Find B, pinned: outdoor towns read 1.2-1.5x brighter at night

After find A's fix, every outdoor town keeps a ratio of about 1.2-1.5
(kladwtwn 1.44, klatrap 1.45, ncr1 1.51, tribe01 1.46, modmain 1.48 - its
roof the worst part). Cause: the classic dims the whole screen by the map
script's night `set_light_level(49)` = 61.75 percent; the 3D night is its own
sun model and sits brighter. Candidate renderer rule: apply the script's night
level as the 3D night ambient floor. Renderer lane, not cut here.

## Find C, pinned: dark-3D outliers

- sfchina2 0.35: the bunker room's lamps are far weaker at night in 3D.
- depolva 0.43 / depolvb 0.49: the Sierra surface at night (script night
  level 40); the 3D reads darker than the classic.

## Missing rows, still open

ardead and vctycocl: the classic capture stalls at the live-check watchdog
step (30 s without progress) on four attempts each, including the walk's own
tiles and the 2026-11-25 build. Their 3D sides are measured (48.08 and 60.21
in the first-pass table). The classic sides stay missing and the rows stay
open - they are not counted as resolved; a live-window capture is the next
thing to try if anyone picks this up.

## Gates

- Exporter unit tests 183 OK (6 new), ctest 35/35.
- Scoped fly tour (4 flip maps + 4 controls) in `tour/`: 0 of 39 spots
  changed. The tour shoots still at full ambient, where the daylight channel
  the fix adds is invisible by design; the night deltas stand in the fix
  table above. The zero matters as the unintended-change gate: the fix moves
  no geometry, atlas or day-hour pixel.
- The trial export ran into `build/cache-285trial` (exporter 1.14.0); the
  shared cache was not touched before the merge. The owner's next export
  rebuilds every scene once: the fingerprint covers the vendor hashes.

## Evidence storage

Git keeps the seven pictures this README shows (about 0.9 MiB); everything
else lives in the evidence release, per the 291 process:
https://github.com/Eugenius-S/f2-reimagined/releases/download/evidence/285-towns-at-night-fa035bd8.zip
(16 sheets: the remaining sweep pairs and the tour's per-map sheets).
Downloaded SHA-256:
a796ff2e558a97543ad36c0c62c607705e25ffc7931dddb8bb8941a4e1e35164.
