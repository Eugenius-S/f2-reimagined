---
id: 310
title: perf-check's noise rule reads the real CPU load and never passes an unknown one
milestone: M1
status: doing
owner: claude
suggested: claude
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [290]
reviewed: no
---

## Goal

perf-check marks a map noisy from a CPU reading that matches the machine's real busy time, and a run with an unknown or noisy reading never prints PASS.

## Context

- Blocks 306's performance gate (PR #517 stays draft until this is reviewed and a repeat measurement runs).
- On 2026-11-17 every perf-check row was `noisy` with `Win32_Processor.LoadPercentage` at 40-55%, while `PercentProcessorTime` read 10-28% and the per-process CPU time summed to about 1.5 of 16 cores. The gap alone does not prove which reading is right.
- `Read-CpuLoadPercent` returns `$null` when the read fails, and a `$null` row counts as clean. A noisy row without a threshold breach still lets the run print PASS.
- 271's `tools/map-health.ps1` (GLM, PR #516) has its own `-NoisyCpuPercent` rule; it is not touched here.

## Acceptance

- A bounded, reproducible probe loads a known number of cores and records every candidate counter next to the per-process CPU time; its output is in this task's run folder.
- The noise input changes only if the probe shows the current one is wrong. Thresholds (`-MinFps`, `-MaxLoadMs`, `-NoisyCpuPercent`) do not move.
- Fail-closed: an unreadable CPU reading is noisy, and a run with any noisy row exits 2 (NOISY), never PASS. `-SelfTest` covers both.
- An independent review of the probe and the rule before 306's repeat measurement.

## Out of scope

GPU or disk contention detection, map-health.ps1, new perf features.

## Log
