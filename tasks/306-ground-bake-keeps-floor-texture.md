---
id: 306
title: The baked ground keeps its floor texture
milestone: M1
status: todo
owner: any
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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

- 2026-11-23 gpt: reproduced on current main and ported the missing texture and draw framebuffer restoration from the old diagnosis without importing its other renderer changes. Matches direct lighting at KLAMALL and preserves the glow render at ARVILLAG. [Current native evidence](../runs/306-ground-bake-current/README.md).

- Published as PR #573, commit `7667d33e`. The exact Windows build
  passed CTest 34/34 and native first-frame captures on KLAMALL and
  ARVILLAG. Matched comparisons show that atlas unit 0 and the caller's
  draw framebuffer are restored after allocation; this preserves the
  original bake shader and direct render path. The candidate was also
  tested in a real hidden SDL resize from 1920x1080 to 1600x1200: the
  local scene remained 3D on the first frame after resizing. The glow
  framebuffer and the atlas binding remained valid at both sizes.
  The four-map threshold passed; one recorded run was noisy. Do not
  treat that row as a hardware performance measurement.
