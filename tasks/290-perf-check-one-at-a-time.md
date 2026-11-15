---
id: 290
title: Speed is measured one run at a time on the machine
milestone: M1
status: doing
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

Two agents never measure speed at once, and a measurement taken while the machine is busy says so.

## Context

- `tools/perf-check.ps1` runs through `tools/dev-snapshot.ps1`, whose slot mutex is per worktree, so three worktrees measure and build side by side. On 2026-11-13 a dev-only change went red on a different map each run (newr1 55 fps, then klatoxcv 43).
- 271 (the all-map table) builds on perf-check: it needs the same guard.

## Acceptance

- perf-check holds one machine-wide named mutex for its whole run and waits for it; the self-test covers the wait.
- The table prints the CPU load seen at the start of each map; a row measured over a stated load is marked noisy and does not fail the gate, it asks for a rerun.
- AGENTS.md Rule 1 says in one line what a noisy row means.

## Out of scope

Making the game faster.

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 6).
