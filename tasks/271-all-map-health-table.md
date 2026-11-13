---
id: 271
title: One command measures every map: load time, frame rate, fallback lines, black frames
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

One command walks every playable map and elevation in the test game and writes a table: load time, average and 1% low frame rate, any 2D fallback line in the log, and the share of black pixels in the 3D frame.

## Context

- `tools/perf-check.ps1` measures four maps. The strategy (2026-11-04) asks for 60+ fps and 2 s loads everywhere, and nobody knows the numbers for the other maps.
- The owner's bar is an 80 fps 1% low; the gate stays at 60 fps and 2000 ms.
- Reuse perf-check's launch and its `perf` lines; the map list comes from the export groups (all 152 playable maps).

## Acceptance

- `tools/map-health.ps1` (or a perf-check switch) runs the whole list unattended, survives a map that hangs (timeout, the row says so) and resumes from where it stopped.
- `runs/271-all-map-health-table/README.md` holds the table for every map and elevation on one main commit, sorted worst first, plus the list of rows over 2000 ms, under 80 fps 1% low, with a fallback line, or over 20% black.
- A self-test covers the table parser; it runs in CI like perf-check's.

## Out of scope

Fixing what the table shows (272, 273, 275).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
