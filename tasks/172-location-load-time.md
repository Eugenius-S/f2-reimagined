---
id: 172
title: A location loads in 2 s from a warm cache
milestone: M1
status: doing
owner: glm
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [127]
reviewed: no
---

## Goal

Walking into a local map with its scene cache already built shows it in 3D
in 2 s or less, measured from the map change to the first full 3D frame.

## Context

- The owner, 2026-11-04: fast loading is critical; a location loads in a
  couple of seconds. The cold cache build (the first export of a map) is
  one-time and is measured separately, not held to 2 s.
- Measure first: time every phase of a map change (the engine's own map
  load, reading the cache files, texture and buffer upload, shader and
  render target setup, the first frame). `ce/src/scene/scene_view.cc` loads
  the floor, walls and projection from `mod-cache/scene/<map>/<elevation>`.
- Likely wins: reading the cache once and in parallel, uploading textures
  without stalls, keeping shaders and render targets across maps.

## Acceptance

- A phase table (before and after) for kladwtwn, newr1, vctyctyd and one
  cave map in the Log and in `runs/172-location-load-time/`.
- A warm-cache map change at 2 s or less on each of them; the cold cache
  build time written next to it.
- ctest and the regression pass.

## Out of scope

Frame rate in play (157), settings changes (156).

## Log

## Log

- 2026-11-06, glm. Measured first: the load chain now reads as one table per
  map change (perfLoadStart at mapLoadByName, marks for map_tiles, map_scripts,
  map_objects, map_elevation, map_load, scene_loaded, first_frame; the scene
  stages floor/walls/projection and their read/decode/upload sub-phases under
  F2CE_PERF=1). Before (kladwtwn / newr1 / vctyctyd / cave0): 2487 / 3377 /
  2332 / 484 ms to the first full 3D frame. The engine's own map load
  dominated: objectLoadAll 1.3-1.9 s, and inside it _obj_rebuild_all_light
  0.8-1.3 s; then the art cache preload ~0.8 s; the scene load 0.24-0.42 s.
- Fixes, each result-identical: (1) the light ray walk's rect unions ran only
  for the rect output the rebuild never reads - skipped when rect == nullptr
  (_obj_rebuild_all_light 843 -> 239 ms on kladwtwn); (2) _obj_insert locked
  art during the load for a sort comparison whose result vanilla never used
  (the TODO: Incomplete block, v11 was never set) - removed, inserts are free;
  (3) the art cache warm spread over a worker pool: cacheLock splits into a
  locked hit path, the size and read procs outside the lock into a private
  buffer, and a locked insert with a double check against another thread
  locking the same key (the eviction and the heap stay under the lock;
  cacheFlushLocked for the insert's last resort); dfileOpen takes a mutex for
  its open-list push (every DFile has its own FILE* and inflate state, the
  reads never share); the preload warms the unique FIDs on up to 4 threads.
  art_preload 810 -> 446 ms (kladwtwn). The vanilla miss path fed the sound
  mixer every few locks; the threaded warm replaced those calls, one
  soundContinueAll on the main thread after the join keeps the loading sound
  moving. Two traps found on the way: soundContinueAll from worker threads
  and the per-read perf log writes multiplied the warm to 5-7 s - the scopes
  moved to aggregates.
- After: 1946 / 2519 / 1757 / 462 ms. kladwtwn, vctyctyd and cave0 hold the
  2 s budget; newr1 lands at 2.52 s (-25%), its remaining breakdown: the
  engine load 1628 (objects 932 with the light spread 297 and ~590 of
  per-object script links and inserts, the preload 525, the entry scripts),
  the scene 405 (the wall and tile PNG decodes are serial), the first frame
  447. The next levers, in order of size: the light spread on worker threads
  (per-thread delta grids - the intensity adds carry no clamps, any fold
  order lands the same totals, the blocked table is per-light), the scene's
  PNG decodes in parallel (pure CPU on private buffers), the per-object
  script link cost. Evidence: runs/172-location-load-time/phases.md.
- 2026-11-08, glm. The planned next lever (the light spread on worker
  threads) was aimed at the wrong cost: with the protos warm the whole
  spread walk runs ~2 ms serially, and a parallel spread over 4 threads
  made objects_light slower (297 -> 390 on newr1), not faster. The
  rebuild's real cost is the lazy proto loads: the walk reads wall
  extendedFlags per visited tile, and every miss pays a .lst and a .pro
  file read, ~2 ms each (219 unique wall protos on newr1, ~0.4 s). The
  lever became the parallel proto warm: protoParseProto (proto.cc) reads a
  proto's file into a fresh Proto and touches no lists, protoInsertProto
  appends a copy of a parsed proto to the lists with protoGetProto's own
  eviction check; the rebuild collects the map's unique wall pids, parses
  them over up to 4 threads (pure file reads) and inserts them on the main
  thread, then runs the vanilla spread loop. objects_light 238 / 295-297 /
  260 / 0.1 -> 48 / 46-48 / 45 / 0.8 ms (kladwtwn / newr1 / vctyctyd /
  cave0). First frame: kladwtwn 1933 -> 1793, newr1 2524-2609 -> 2319-2397,
  vctyctyd 1827 -> 1508, cave0 638 -> 619. The spread is pixel-identical,
  0 changed px against the before snapshots on all four maps; ctest 20/20.
  newr1 still needs the scene PNG decodes in parallel and the per-object
  script link cost. Evidence: runs/172-location-load-time/phases.md.
- 2026-11-08, glm. The scene PNG decodes left the serial path. Walls: the
  batch loop now only registers one decode per page; up to 4 workers read
  and decode the page PNGs on private buffers (the file reads moved in
  too), the main thread joins, fails on the first error in file order, and
  hands the bytes to the first batch naming the page, so texture ownership
  stays as before and the upload loop is untouched. The walls_decode scope
  now covers read + decode and reads as the longest page instead of their
  sum (240 -> 157 ms on newr1; walls total 320 -> 212, scene_total 437 ->
  330). Floor: roof_fringe.png decodes in one worker while tiles.png
  decodes on the main path, joined right after so no early return outruns
  it. First frame: kladwtwn 1793 -> 1710, newr1 2319-2397 -> 2230-2280,
  vctyctyd 1508 -> 1637 (run noise of the same size as the lever is back;
  the scene stage itself is down ~90 ms there too), cave0 ~619 -> ~700
  (same noise). Pixel-identical, 0 changed px on all four maps; ctest
  23/23. newr1 needs the per-object script link cost and the first-frame
  chunk next. Evidence: runs/172-location-load-time/phases.md.
- 2026-11-08, glm. The scene PNG decodes left the serial path (walls:
  one decode per page registered by the batch loop, up to 4 workers read
  and decode them on private buffers, the upload loop untouched; floor:
  roof_fringe.png decodes in one worker while tiles.png decodes on the
  main path). Walls 320 -> 212 ms and scene_total 437 -> 330 on newr1;
  the walls_decode scope now reads as the longest page (157 ms) instead
  of the sum (~240 ms). First frame: kladwtwn 1793 -> 1710, newr1
  2319-2397 -> 2230-2280, vctyctyd/cave0 unchanged within their noise.
  Pixel-identical, ctest 23/23. Evidence:
  runs/172-location-load-time/phases.md.
- 2026-11-08, glm. Probes that cut the next lever candidates: the
  per-object script link is free (6341 objects at 0.00 ms; the old Log's
  ~590 ms was the lazy proto loads, already fixed), so lever 3 from the
  list is dead. The art warm does not scale past 4 threads (8 threads:
  390 -> 370 ms on newr1, noise) - the cap stays. objects_load ~560 ms
  is the engine's serial objectRead over one stream. New load-chain
  marks (ff_present_top, ff_sync_done, ff_draw_begin, ff_shadow_done,
  ff_bake_done) split the scene_loaded -> first_frame gap: on newr1 the
  shadow map + first scene draw cost ~65 ms and the remaining ~450 ms is
  the game loop's own work between presents (in a dev-snapshot run that
  includes the preview policy and the travel retry - the owner's live
  game may differ). The remaining lever list: attribute the ~450 ms
  between-presents chunk, then the serial objectRead. Pixel-identical
  marks, ctest 23/23. Evidence: runs/172-location-load-time/phases.md.
- 2026-11-08 claude: owner order 2026-11-08: glm, second after 239.
