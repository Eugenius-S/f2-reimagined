---
id: 290
title: Speed is measured one run at a time on the machine
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
- 2026-11-15 glm
  Done: perf-check holds the machine-wide `Global\fallout2-ce-perf-check`
  mutex for its whole run and waits for a busy lock (it says so while it
  waits, gives up after -WaitMinutes); the table prints the CPU load seen at
  the start of each map, a map over -NoisyCpuPercent (30) is marked noisy,
  and a threshold breach there prints NOISY and exit 2 (rerun) instead of
  FAIL; thresholds untouched, a noisy run never prints PASS. AGENTS.md Rule 1
  states in one line what a noisy row means. Self-test covers the noise
  classification and the lock wait (a holder process takes a test mutex, the
  probe sees it busy and free again).
  Check: `tools/perf-check.ps1 -SelfTest` ok; unittest suites ok (148 + 12);
  ctest 28/28; two competing runs serialized (A 13:46:10-13:48:13, B
  13:46:15-13:50:17 with the waiting line, measured runs strictly
  sequential); a clean full run PASS 4/4, cpu 16-24, 1% low 84.4-130.7 fps,
  load 686-1664 ms; CI green on 60f85062. The rerun path held end to end:
  a run with every row failing over a stated load (-MinFps 200
  -NoisyCpuPercent 0, test parameters only) printed NOISY per map, "not a
  PASS", and exited 2.
