# 157: frame pace measurements

Setup: `tools/dev-snapshot.ps1 -ConfiguredDisplay -ReuseTestConfig`, off-screen
1920x1080, `[3d] fps_limit=0`, shadows and sun on, `F2CE_PERF=1`,
`F2CE_PERF_HOLD=25`. The first 3 s and the last 0.7 s (the snapshot capture) of
each run are left out. "Before" is this branch's build with the three fixes
taken out (main's frame work plus the profiler), run back to back with
"after" on the same map, since the machine is shared and absolute numbers
drift up to 2x between runs.

1% low here is the mean of the slowest 1% of frames, in ms.

| Map | Build | Frames | Avg ms (fps) | p50 ms | p99 ms | 1% low ms | Worst ms | Frames > 25 ms |
|---|---|---|---|---|---|---|---|---|
| kladwtwn | before | 2101 | 10.47 (95) | 9.53 | 41.1 | 52.2 | 82.6 | 29 |
| kladwtwn | after | 2866 | 7.68 (130) | 7.17 | 16.1 | 40.1 | 77.6 | 27 |
| klamall | before | 2161 | 10.18 (98) | 9.83 | 15.3 | 32.8 | 45.5 | 13 |
| klamall | after | 3009 | 7.31 (137) | 7.08 | 11.0 | 30.5 | 59.5 | 19 |
| newr1 | before | 1215 | 11.83 (85) | 7.30 | 42.7 | 44.3 | 46.2 | 197 |
| newr1 | after | 2806 | 7.84 (128) | 7.14 | 15.2 | 31.7 | 70.0 | 16 |
| kladwtwn fly | before | 2734 | 8.20 (122) | 7.31 | 18.6 | 51.9 | 363.9 | 22 |
| kladwtwn fly | after | 2581 | 7.89 (127) | 7.31 | 15.0 | 37.1 | 71.0 | 19 |

newr1 again, the fixes before this one against all three (two pairs):
>25 ms frames 260 and 173 before the text fix, 8 and 2 after.

## Causes found

- Floating text (a character's bark) in the game view: each frame the bark is
  blitted into the tile window, and the sharp-text layer's copy looked up
  "what was under this pixel" by a linear scan over the destination's text
  records for every glyph pixel. The tile window keeps last frame's bark
  pixels, so the scan was quadratic: 4 to 30 ms per frame while a bark shows
  (newr1: 25 to 30% of frames over 25 ms). Fix: one hash index per copy,
  same answer (newest record, first pixel). `ce/src/scene/text_layer.cc`.
- Object cards wholly off the view were still submitted: draw submit went
  3.8 to 0.9 ms (game camera), 4.0 to 2.4 ms (flight).
  `cullStaticObjectDraws` in `live_objects.cc`, used by `scene_draw.cc`.
- The muzzle-flash light walk looked up the shown art of every object; only
  critters fire. About 0.5 ms a frame in a town. `scene_lights.cc`.

## Left as is

- The rest of the >25 ms frames, about 15 to 30 per 25 s run: about a quarter
  come with a stdout flush to the file `dev-snapshot.ps1` polls (a ~32 ms
  stall, harness only), the rest are bursts where every stage of the frame
  slows at once, which a parallel jitter probe saw too (other work on the
  shared machine). The flight's 300+ ms worst frame before is the snapshot
  capture landing in the window.
- So the average is over 120 fps on every measured map, but the 1% low is
  still 30 to 40 ms under this harness; whether that holds on a quiet machine
  without the redirected stdout is not measured.

## Checks

- Fly tour, the build without the fixes against this branch, on kladwtwn,
  klamall, klacanyn, klagraz, klatrap, klaratcv, klatoxcv, newr1, newr2,
  vctydwtn and modmain: 0 of 57 spots changed (`changed.jpg`). denres1 (also
  in the regression, on both builds) and sfelronb (the side without the
  fixes; this branch's side was not reached) hang at startup with no
  snapshot, so the full list did not finish.
- Regression: 153 of 154 spots pass with 0 changed pixels; den-residential
  (denres1) gets no snapshot, the same with the build without the fixes.
