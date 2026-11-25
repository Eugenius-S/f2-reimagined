id: 326
title: The ground bake repeats only when its picture changes
milestone: M0
status: doing
owner: glm
suggested: glm
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no

# 326: The ground bake repeats only when its picture changes

## Goal

At the Klamath bathhouse (KLADWTWN e0, hero 18479, camera 18271) the game
holds 47.8 FPS on the owner's machine (frame average 20.92 ms, build
1c893f5af), under 60. Bring that spot back over 60 FPS without changing the
picture or the VSync setting.

## Context

- The owner's repro: his Steam install's `screenshots\20261124-234309-
  kladwtwn-e0.png` + `.json` (scene-gl, sun shadows, glow 2.0, 1720x720
  logical at scale 2.0, borderless 3440x1440, zoom 1.024).
- The owner's display runs 144 Hz with VSync on (the default
  `settings.screen.vsync`): a frame cost between 13.9 and 20.8 ms presents as
  3 vblank intervals, 20.83 ms, 47.8 FPS. The frame cost on the same spot
  measures 14.2-15.6 ms in the background harness, just over the two-interval
  budget (13.89 ms), so the queue holds.
- Frame profile on that spot (F2CE_PERF, GPU timers): floor 6.0 ms, walls
  3.9 ms, objects 2.3 ms of a 14.4 ms GPU frame. The floor pass is mostly the
  157 ground bake: a half-resolution render of the whole floor with the light
  loop and the 25-tap shadow disk, repeated every frame even though its inputs
  stand still while the player watches. With the bake off the floor costs
  14.8 ms (42.9 FPS), so the bake pass is worth about 5 ms per frame.
- The previous release (pre-586, build 039d25da3) measures the same 66.9-68.6
  FPS on the spot: a standing cost, not a recent regression.

## Acceptance

- Three comparable runs on the repro spot with the owner's settings hold
  60+ FPS worth of frame budget: frame average under 13.89 ms (two 144 Hz
  vblank intervals) with margin.
- The picture does not change: an A/B devshot against a clean main exe is
  byte-identical on the repro spot, and the fly tour over the Klamath maps
  shows no changed spot.
- VSync stays on; no settings change.

## Out of scope

- Camera pans and walks re-bake every frame (the bake is screen space; its
  matrix changes per frame then), so the standing spot is the fixed case; a
  world-anchored bake would be a separate task.
- The walls (3.9 ms) and objects (2.3 ms) per-pixel shadow disks.

## Log

### 2026-11-25 glm
Started: reproduced the drop on an isolated RP 2.4.34 install
(build/test-install), measured the baseline and the previous release, found
the bake's per-frame repeat, and wrote the cache key (scene_draw.cc) over the
already-present change signals (shadow map redraw, light inputs, receiver
group). Three after runs: 78.5-85.5 FPS, frame average 11.7-12.7 ms.
Next: the fly tour, then the draft PR.
Check: `runs/326-ground-bake-cache/` and the PR body.
