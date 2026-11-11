---
id: 232
title: The launcher's progress bar follows the real download and map build
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

During the install the launcher's progress bar shows the real work: the
download by bytes, then the map build by scenes, and the log shows life while
the maps are built.

## Context

The owner: the bar says "about 20 minutes" and reaches the end almost at once,
then sits there. Cause: every worker log line added 2%, and the download alone
prints about 32 lines. A second screenshot: during the build the log shows
nothing after "making the 3D scene data for: all", because the exporter ran
without the worker's output handles and its lines never reached the GUI.

## Acceptance

- Download is 0 to 10% by bytes (`  ... N / M MB`), unpack 10%, the scene
  export 12 to 99% by scene lines against `{"maps_total": K, "scenes_total": N}`
  (failed scenes count), 100% when the worker succeeds (also when nothing had
  to be rebuilt).
- The exporter's output reaches the GUI log; the raw scene JSON lines are
  hidden there, each finished map adds one line ("klamall: 3D ready", "up to
  date", "3D failed"; Russian too) and the status line reads "Building 3D: N
  of M maps".
- `export.py --maps` prints the totals line first and `FAILED` lines at once.
- Tests: `test_export` (totals line), `launcher_progress_tests` (the parser).

## Out of scope

A real GUI run of the long first install (the parsing is tested, the window
code was read).

## Log

- 2026-11-08 claude: tracker in `ce/src/launcher_progress.h`, wired in
  `launcher_ui.cc`; `launcher_main.cc` prints the download total and passes
  its output handles to the exporter (not to the game).
- 2026-11-08 claude: the owner added: per-map lines in the GUI log and the
  "Building 3D: N of M maps" status line; done in the same PR.
