---
id: 272
title: Every map loads in 2 seconds and holds an 80 fps 1% low
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [271]
reviewed: no
---

## Goal

No row of 271's table is over 2000 ms of load or under an 80 fps 1% low.

## Context

- Take the rows from `runs/271-all-map-health-table/README.md`, worst first. Group them by cause before fixing: one cause is one PR.
- Past causes are in the Logs of 155 and 157 (the atlas build, shadow passes, stdout flushes, the test game's own snapshot).
- Renderer speed work never runs alongside a geometry PR that touches the same files.

## Acceptance

- Each PR names its cause, the maps it covers, and the table rows before and after (three runs each, the machine idle).
- 271's table rerun on the final commit has no row over the bar, or each remaining row has its cause pinned in the Log with the measurement that shows it.
- ctest and the pixel regression pass on every PR.

## Out of scope

Lowering quality settings by default to reach the number.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
