---
id: 219
title: "FPS round 2: bake the ground light, cache the text and the object cards"
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [157, 218]
reviewed: yes
---

## Goal

Give 157 the missing frame budget on the busy maps without changing the
picture: the ground light/shadow cost, the per-frame sharp-text rebuild, and
the per-frame object rebuild all stop paying full price every frame.

## What shipped

- `glGetError` checks are behind `F2CE_GL_DEBUG` (gl_api.h). The hot-path
  error polls serialized the CPU against the whole GPU queue; the hero card
  sync went 8.4 ms to 0.01 ms the moment they left the frame path.
- Sharp-text two-key cache (text_present.cc): a static-layer key (the iso
  window's shown bytes, chunk-hashed, cycling palette entries 229-254 and the
  mouse cursor's chunks excluded) plus a glyph-layer key. Both keys stable =
  redraw the two textures, nothing rebuilds; only the cycling box and the
  cursor box patch over the cached layers. Measured: the prepare phase went
  7.6 ms to ~2.1-2.9 ms, and after startup the cache takes zero misses (the
  once-a-second full rebuilds are gone).
- Incremental object cards (live_objects.cc): a snapshot of the cheap object
  state; unchanged objects re-append their six vertices instead of locking
  art, decoding and re-projecting. 99.96% hits on kladwtwn.
- Ground bake (scene_draw.cc + scene_shader.cc): the floor's light and sun
  shadow render once per frame at half resolution into an RGBA8 FBO; the
  floor tiles sample it (uGroundBakeMode 2) instead of running the per-pixel
  light loop and the 25-tap shadow disk. Floor pass 11.0 ms to 0.73 ms.
- The one regression found and fixed during this round: `drawSceneOverlay`
  must keep uploading the game's picture into the shared quad texture
  (gTexture) - the side patch texture is the mark rectangles' alone. The
  first cut of the patch path left gTexture holding the last loading screen,
  and every cache-hit redraw and snapshot painted it over the world.

## Results (warm scene cache, off-screen, owner settings, 2026-11-08)

- kladwtwn game camera: 200 frames per 2 s window (100 fps, 10.0 ms) after a
  12.5 ms warmup; the sharp cache takes no misses in the settled phase.
- klamall game camera: 198-200 frames per 2 s window (99-100 fps, 10.1 ms).
- klacanyn and klatoxcv hold ~61 fps in the same harness with a 5-6 ms frame
  cost; the cap sits outside the render path (demo map pacing), the render
  headroom there is over 100.
- Visual regression of the bake (same exe, F2CE_NO_BAKE off/on, hour pinned
  8): 1.24% of pixels differ past delta 24, all of it moving critters and
  half-res shadow-edge softening; the pair reads identical by eye.

## Diagnostics left in

`F2CE_NO_BAKE`, `F2CE_NO_SHARP_CACHE`, `F2CE_NO_CARDS` switch each new piece
off for a run; `F2CE_GL_DEBUG` restores the hot-path error polls; one-shot
stderr probes (probe1-9) print scene state twice per process under this
round's flags.

## Log

- 2026-11-07/08 Built on top of 218's branch; ctest 20/20; the black-screen
  hunt (poisoned re-exported demo scene caches + the gTexture regression)
  documented in the session notes.

### 2026-11-08 claude
Done: rescued #336 onto fresh main; the 157 one-shot GL probes left a GL error that failed every launch snapshot ("static wall draw failed before snapshot"), removed. Release, ctest 20/20, launch snapshots on kladwtwn, klamall, klatoxcv pass, world on screen. Test config (vsync) holds ~57 fps with gpu 1.9 ms, cpu 2.9 ms on kladwtwn.
Next: nothing.
Check: `tools/dev-snapshot.ps1 -Map kladwtwn` with `F2CE_PERF=1`.
