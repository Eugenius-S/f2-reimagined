---
id: 157
title: Steady 90+ FPS: no uneven frames in the game view or in flight
milestone: M1
status: todo
owner: claude
suggested: claude
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: [155]
reviewed: yes
---

## Goal

The game holds a steady 90+ FPS on the owner's machine with the default 3D
options on: no uneven pacing, no periodic hitches, in the game camera and in
flight.

## Context

- The owner, 2026-11-02, on main `e8f168a2` after 155: it still runs
  unevenly ("как-то криво идёт"); they want a smooth 60.
- 155 left two leads (`runs/155-performance/results.md`): ground shadows
  cost 6.6 ms against 0.9 ms with shadows off; the 1% low frame is set by
  game loop stalls outside the renderer.
- Check the pacing as well as the cost: the frame limiter
  (`sharedFpsLimiter`), vsync and swap interval, CE's tick-based game loop
  against the render rate, work done every Nth frame. The profiler is
  `F2CE_PERF` (docs/p1-devtools.md); the FPS counter is 127.

## Acceptance

- A frame time trace of 30 s in Klamath downtown (game camera, walking, and
  a flight) before and after, in `runs/157-steady-60-fps/`: average at or
  under 16.7 ms, 1% low and the worst frame named, each cause of a spike
  written in the Log with its fix.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Lower visual quality as a default; display changes without restart (156).

## Log

- 2026-11-02 Cut from the owner's find; follows 155.

## Log

- 2026-11-07 (main 990d7756, warm scene cache, off-screen 1920x1080, shadows
  and glow on): kladwtwn game camera 46.2 fps average (21.6 ms, 1% low 40.0,
  worst 34 ms); kladwtwn fly 60.7 fps, riding the 60 fps limiter; newr1 game
  camera 28.5 fps (35.0 ms, 1% low 15.6 fps, worst 66 ms). The owner raised
  the bar the same morning: below 60 is not acceptable, below 120 is not
  comfortable; the target of this task moves to a steady 120+ on the owner's
  144 Hz screen.
- F2CE_PERF pass breakdown (shadows and glow on): the floor pass is the top
  GPU cost, 7.3-10.0 ms on kladwtwn and 12.5 ms on newr1, about half the GPU
  frame; walls 2.3-3.4 ms, flat objects 1.5-1.7 ms, objects 1.2-1.9 ms,
  hero egg glass up to 2.8 ms on newr1, GPU frame 14-22 ms, CPU submit
  2.5-5.3 ms. On newr1 the frame averages 55 ms against 22 ms of GPU: about
  33 ms per frame of stalls outside the GPU (the 155 finding holds). One
  883 ms single-frame spike on kladwtwn, the same stall class as task 206.
- First change: the game view paces to a new [3d] fps_limit setting (60
  default, 0 removes the limit) with a sleep-plus-spin throttle, so the limiter
  stops hiding the render headroom behind the classic 60.
- 2026-11-07, owner order: the 60 cap must go (more is welcome), and until
  every location holds a steady 60+ the next phases do not start. This task
  is the gate: the floor-pass GPU cost and the off-GPU stalls above are the
  remaining work.
- 2026-11-08 (219 landed on this branch): kladwtwn and klamall hold 100 fps
  off-screen with the owner's settings (10.0-10.1 ms, the sharp cache takes
  no misses after startup); the ground light/shadow bake cut the floor pass
  11.0 to 0.73 ms. Still open for the gate: a real-window validation on the
  owner's machine (the off-screen harness capped two demo maps at ~61 by
  their own pacing) and the 120 comfort target.
- 2026-11-08 claude: owner order 2026-11-08: the bar is 90+ FPS on every location; claude does it.
- 2026-11-09 claude (PR #372 merged, main 0a825d85a): egg passes scissored to the egg rect, sharp-text scan updated in place with a whole-row skip, the object walk's boundary-mask path cached per fid. kladwtwn same session, main vs branch: sharp 0.92-1.05 to 0.26-0.29 ms, object walk 2.7-4.7 to 2.0-2.2 ms, refresh 5.2-5.5 to 4.0-4.7 ms. Note: the default [3d] fps_limit is 60 and a config without the key (test runs, the Steam install's cfg) holds 60. With fps_limit=0, off-screen 1920x1080: kladwtwn 10.0-15.2 ms per 2 s window (66-100 fps, one 30 ms window), klamall 11.2-13.2 ms (75-90 fps, one 39 ms window). Not yet a steady 90+: present (draw submit and swap) is 9-15 ms and spiky; the next work is the spike windows and the submit cost. Open question for the owner: the default fps_limit (60 today).
- 2026-11-11 claude (audit, folded in): (1) four frame timers disagree: the on-screen readout (`ce/src/svga.cc:713-740`, a 500 ms window, no 1% low), the frame log, the owner shot's ring (`perf.cc:388-423`) and devperf; the readout should read the ring. (2) `perf.cc:65` `gRun` keeps every frame of the run and is sorted every 2 s window: cap it to the last 10000. (3) `walls.cc:475-489` computes a 4-thread cap and then starts one thread per page.
- 2026-11-12 claude (#476): `tools/perf-check.ps1` measured the wrong frames. Of about 500 play rows only the last 60 were the named map; the rest were the menu and the start map, and the first three rows after the load were the frames that build the 3D scene. That is why the 1% low read 17-21 fps on every map. Now only the play rows after the last load row count, past their first second, and the game is held on the map for 10 s (about 1350 rows). Two runs on main c0b5eb8ff, shadows off as in the test config: average 130-139 fps on all four maps; 1% low kladwtwn 47 and 67, newr1 74 and 26, vctyctyd 118 and 109, klatoxcv e2 82 and 65; load 1517-1550, 2043-2082, 1394-1438, 742-767 ms. Still red for real: newr1 load just over 2 s, and a 1% low that jumps between runs.
- 2026-11-12 claude, the jump's cause (#476): the test game's own log writes. A build that timed the loop inside perf-check showed `devtools::update` taking 32.1-32.6 ms every 60th frame: the `DEV_WATCHDOG state` line and its flush. A flush of a log on the project disk costs about 32 ms (0.4 ms when the log is on the system disk, which is why single runs with a scratch output folder were clean). The state line now goes out every 2 s by the clock. perf-check after it, same main: 1% low kladwtwn 96 (was 26-67), vctyctyd 65, klatoxcv e2 65, newr1 25; average 123-137 fps. What is left in the sample is the same cost from the profiler's own files: `frame_log.csv` flushed every 100 rows, the `devperf.log` windows and the `perf present` line every 2 s; those frames show 35-41 ms with no span over 5 ms. None of this runs in the player's game (no log file, no dev tools). Next: send the measured run's logs to the system temp folder or flush them from one place once per few seconds, then judge the real 1% low; and the newr1 load, 2.04-2.10 s against the 2 s bar.
- 2026-11-12 claude: the load clock no longer counts the test harness's own screenshot (`perfLoadSkip` around `takeSnapshot`, about 400 ms on newr1), and the every-120-frames and every-2-s `perf ...` diagnostic prints lost their own `fflush`; the state line still flushes the buffer every 2 s. Three perf-check runs in a row pass on all four maps: 1% low kladwtwn 81-91, newr1 85-95, vctyctyd 118-129, klatoxcv e2 113-120 fps; load 1305-1356, 1669-1744, 989-1053, 588-633 ms. The max frame still shows one 38-42 ms frame per run (the remaining 2 s flush); it sits outside the 1% low. Left: newr1 load has 250 ms of headroom under the 2 s bar.
