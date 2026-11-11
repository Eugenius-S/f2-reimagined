---
id: 250
title: The frame-time log measures right, a fine clock, no stale first frame, load frames marked
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

`frame_log.csv` and its summary can be trusted to judge the 90 FPS bar and
to find stalls.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 3).
  Stands alone: 127 (the counter) is finished work waiting to be closed,
  157 is the renderer's speed itself.
- `ce/src/fps_counter.cc:90-96` takes the frame time from `SDL_GetTicks`:
  whole milliseconds. An 11.1 ms frame reads 11 or 12, up to 9% off.
- `lastFrameTicks` is a function static that is never reset. Turn the log
  off, turn it on a minute later: the first row of the new log is a minute
  long and `worst_ms` in the summary is false.
- Loading-screen presents land in the same rows as play frames, so the 1%
  low mixes load time into frame rate.
- `ce/src/scene_renderer/perf.cc:394` already uses
  `SDL_GetPerformanceCounter` for the owner shot's numbers.

## Acceptance

- The log uses the performance counter; rows carry sub-millisecond values.
- Opening a log starts a fresh interval: the first row of a reopened log is
  never longer than the frames after it because of the pause.
- The CSV has a third column, `load` (1 while the file read progress handler
  drives the present, else 0); the summary counts play frames only and
  prints the load frames' count on its own line.
- The summary math (average, 1% low, worst) is a pure function with a ctest
  case: a made-up series with one long first frame and three load frames
  gives the expected numbers.
- Only `ce/src/fps_counter.cc`, its header, its test and the CMake line
  change; `svga.cc` gets at most the one argument it has to pass.

## Out of scope

The on-screen readout and the other frame timers (157); the renderer.

## Log

- 2026-11-11 claude: cut from the audit.
- 2026-11-11 glm
  Done: `ce/src/fps_counter.cc` times frames with `SDL_GetPerformanceCounter`
  (sub-millisecond rows), resets the interval when a log opens (a pause with
  the log off never lands in the first row), and takes one argument from
  `ce/src/svga.cc` (`fpsCounterOnFrame(fileReadProgressHandlerActive())`,
  the only change there). Rows carry `frame,ms,load`; the summary math
  moved into the pure inline `frameLogSummary` (`ce/src/fps_counter.h`):
  play frames only, load frames counted separately and printed on their own
  line; `ce/tests/fps_counter/fps_counter_test.cc` pins it with one long
  first frame and three load frames; one CMake block registers
  `fps_counter_tests`.
  Check: `ctest`: 27/27 (the new test included). A background run on
  kladwtwn with `--dev-frame-log` wrote `frame_log.csv`: header
  `frame,ms,load`, row 0 = `0,0.000,0` (fresh interval), 1800 rows with
  168 load rows (the loading-screen presents, ms 2..23), play summary:
  average 103.7 fps, 1% low 28.0 fps (35.8 ms), worst 336.0 ms. The
  engine prints and writes the same numbers in `frame_log.summary.txt`
  when a log closes; a background run ends by a process kill, not a clean
  `gameExit`, so the numbers above come from the same pure formula the
  test pins, applied to the CSV rows.
  Next: 251.
