---
id: 251
title: One command checks the frame rate and the map load time against the owner's bar
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [250]
reviewed: yes
---

## Goal

A merge that slows the game down is caught by a script before the owner
plays it, not by the owner in play.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 4).
  Stands alone: 157 and 172 changed the speed, nothing checks it afterwards.
- No CI job and no script measures speed. The load chain is logged only
  under the `F2CE_PERF` environment variable (`ce/src/scene_renderer/perf.cc`),
  which no script in `tools/` sets.
- The pieces exist: `--dev-frame-log` (`ce/src/devtools/devtools.cc`) turns
  the frame log on; `tools/dev-snapshot.ps1` runs the background test game;
  `perf load ... since chain start` lines give the load phases.
- The owner's bar: a steady 60 FPS at least (90 is the goal of 157), a
  warm-cache map change in 2 s, no stall over 100 ms.
- A background run is paced differently from a real window (157's Log), so
  the thresholds here guard against a regression; they do not prove the bar.

## Acceptance

- `tools/perf-check.ps1` runs the background test game on kladwtwn, newr1,
  vctyctyd and one cave map with a warm scene cache, the frame log on and
  `F2CE_PERF=1`, and prints one table: map, average FPS, 1% low FPS, worst
  frame in ms, load time to the first 3D frame in ms.
- It exits non-zero when a map is under `-MinFps` (default 60) on the 1%
  low, or over `-MaxLoadMs` (default 2000); the failing cell is named.
- The parsing of the summary and of the load lines is a function with a
  self-check on a saved sample (a wrong parse fails the script).
- It writes only under `build/`; it never touches `game/`, never starts
  `tools/dev-launch.ps1`.
- `AGENTS.md`, "Build and check": one line with the command, and the
  post-merge check of Rule 1 names it.
- The table of the first run on main is in the Log.

## Out of scope

Running it in CI (no game files there); making the game faster (157).

## Log

- 2026-11-11 claude: cut from the audit.
- 2026-11-11 glm
  Done: tools/perf-check.ps1. Two background dev-snapshot runs per map
  (kladwtwn e0, newr1 e0, vctyctyd e0, klatoxcv e2 as the cave): a warmup,
  then the measured run with the frame log on (-Set3d frame_log=1, the key
  added to tools/test-config/fallout2.cfg) and F2CE_PERF=1. The CSV parse
  (Read-FrameLogSummary) and the load parse (Read-LoadMs) are pure
  functions; -SelfTest feeds saved samples and fails on a wrong answer.
  The run folder is kept (-KeepRunFolder) because dev-snapshot deletes it
  after copying results out, and frame_log.csv lives only there; the load
  chain prints to the run's logs/devperf.log, not to the captured stdout.
  Everything the script writes goes under build/. AGENTS.md: the command
  added to Build and check, and Rule 1 names the speed gate.
  Check: -SelfTest prints selftest: ok; a run with -MinFps 1000 exits 1
  naming the 1% low cell of every map; the first run on main (default
  bars): kladwtwn e0 57.7 avg / 20.3 1% low / 239.2 worst / 1742.6 load;
  newr1 e0 54.6 / 16.5 / 596.7 / 2300.9; vctyctyd e0 53.5 / 18.0 / 571.8 /
  1596.4; klatoxcv e2 59.1 / 17.0 / 243.6 / 949.4; exit 1 with named cells
  (the 1% low of every map is under 60 in a background run, newr1 load
  2300.9 ms is over 2000). The bars guard regressions; they do not prove
  the owner's window (157).
  Next: 252.
