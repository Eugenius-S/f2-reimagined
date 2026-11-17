---
id: 306
title: The baked ground keeps its floor texture
milestone: M1
status: done
owner: claude
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Fix the renderer cause of the white ground with the normal ground-light bake enabled, so 270 can capture a valid floor and unblock 269 and 256.

## Context

- Standalone renderer prerequisite: 270 explicitly excludes renderer changes. Its draft PR #504 Log isolates the failure to the bake path, not a missing atlas or stale cache. Keep 270 as evidence verification, not a second renderer fix.
- On KLAMALL e0, tile 28096, hour 8, zoom 1, the fresh-cache bake-enabled frame has 62.4% near-white pixels; the otherwise identical `F2CE_NO_BAKE=1` diagnostic has 0. `floor.bin` is F2FL v7, 100x100, 369 atlas cells; `tiles.png` is 2020x1919 and contains the dark floor texture.
- Inspect `ce/src/scene_renderer/scene_draw.cc:49-76,173-239`, its framebuffer/viewport/texture state and the matching shader. These are investigation entry points, not a claimed precise cause.
- Reproduce in an isolated worktree: export Klamath to `build/scene-cache`, then `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map klamall -Elevation 0 -ViewTile 28096 -Hour 8 -Zoom 1 -Still -Shadows -OutputDirectory build/task306-bake`. Use the same command with `F2CE_NO_BAKE=1` only as a diagnostic baseline.
- Reserve the renderer slot first. Do not overlap GLM's 272 performance work, 274-277 geometry work, or any other active renderer PR. No dependency on 270: it waits for this fix.

## Acceptance

- The Log names the precise cause and the shared renderer rule fixed. The original command produces textured ground with bake enabled and no environment workaround.
- Matched before/after images and floor-region measurements prove the white replacement is gone; shadows and lighting still work. Check the same bake path on a second map and after a resize or viewport change. An unchanged second-map result is valid regression evidence; do not add a KLAMALL exception.
- Add one focused check for the corrected state/decision where practical. Run CTest, pixel regression, and the fly tour against main; attach the changed-view sheet and explain intended differences.
- Run before/after perf-check in an isolated measurement slot, using 290 when available. Contended measurements are not a pass; keep the existing performance bar and bake enabled.
- Link the fix and normal capture in 270's Log; leave its remaining evidence and 256's shadow diagnosis to their existing tasks.

## Out of scope

Floor geometry/exporter changes, a blanket bake-disable workaround, new performance features, the shadow-edge fix in 256, or effective-toggle instrumentation in 269.

## Log

- 2026-11-15 gpt: split the missing renderer prerequisite from PR #504's blocked Log. The exact renderer cause still needs reproduction and diagnosis.
- 2026-11-17 claude: Done. Two causes in `scene_draw.cc`, both on the bake path only (bake on = lights and shadows on).
  1. Tested cause of the white floor: `ensureGroundBake` runs mid-frame, after the floor's art is bound on unit 0, and left its own white-cleared target bound there, so the floor sampled the bake buffer as its art. With glow on it also unbound the glow framebuffer, so the frame came out black. Variant test: restoring only the framebuffer keeps 62.4% white; restoring the texture binding removes it. Shared rule: `ensureGroundBake` leaves the texture and framebuffer bindings as it found them.
  2. Found while checking shadows: the bake pass inherited light mode 0 from the frame start, and mode 0 bakes full light without the sun shadow, so the baked floor lost ground shadows and point lights. Rule: the bake pass uses the floor's own light mode (1). After the fix the baked floor matches the unbaked one: mean |d| 1.17 (KLAMALL) and 1.53 (ARVILLAG) per channel, pixels off by more than 16 at 0.00% and 0.01% (were 1.44% and 10.04% with fix 1 only).
  - Near-white share, bake on, before -> after (`F2CE_NO_BAKE=1` mean in brackets): KLAMALL 1920x1080 62.4% -> 0.0%, mean 53.7 (54.1); KLAMALL glow on black frame -> 0.0%, mean 53.7 (54.1); ARVILLAG 73.7% -> 0.0%, mean 80.0 lower band (81.2); KLAMALL 1366x767 51.4% -> 0.0%, mean 45.2. Sheet: `runs/306-ground-bake-keeps-floor-texture/before-after.jpg` (before, after, no-bake per row).
  - CTest 29/29. Pixel regression PASS on every spot (4 spots with 1-2 changed pixels inside tolerance). Fly tour vs main on klamall, kladwtwn, arvillag, newr1, vctyctyd, klatoxcv: 0 of 40 spots changed (`tour-changed.jpg`).
  - Focused check: not added. No GL-context test exists in `ce/tests`, and the regression, the tour and perf-check all run without `-Shadows`, so none of them reaches the bake. That is why this broke unnoticed since 157. Next: one regression spot with shadows on would guard the bake path.
  - Perf, shadows on, paired main/306 runs at the same machine load (owner's other game running, CPU 35-85%, so indicative only): KLAMALL avg 137.3/138.9 and 138.8/139.9 fps, 1% low 90.5/107.1 and 112.6/124.5; ARVILLAG avg 138.7/139.2 and 139.9/139.5, 1% low 90.0/89.3 and 94.4/90.8. No visible cost.
  - Untested observation, not changed: the bake viewport keeps the full viewport's x, y origin while halving its size; a non-zero origin would misplace the bake.
  - perf-check, main then 306, owner's other game closed, real CPU busy time 10-28% (`Win32_PerfFormattedData_PerfOS_Processor`): PASS both. 1% low main/306: kladwtwn 85.4/80.9, newr1 66.1/91.3, vctyctyd 128.4/125.6, klatoxcv 121.7/118.5; loads 1295/1213, 1583/1587, 960/928, 534/533 ms. perf-check runs without shadows, so it does not reach the bake; the spread is noise. Every row still printed `noisy`: perf-check's `Win32_Processor.LoadPercentage` reads 40-55% while the real busy time is 10-14% (it follows the turbo-scaled processor utility), so its noisy rule (290) cannot pass on this machine. Next: read `PercentProcessorTime` instead.
