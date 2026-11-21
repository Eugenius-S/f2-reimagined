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

- 2026-11-17 claude: Probe and fix in PR #521, waiting for an independent review.
  - Probe `runs/310-perf-noise-reads-real-cpu-load/probe.ps1` (holds the perf lock, at most half the cores, about 80 s): 3 valid runs of k = 0, 2, 4, 8 busy processes in `probe-2026-11-17.txt`. `PercentProcessorTime` follows the added load and sits 2-12 points over the per-process sum (kernel and protected processes, as expected for a lower bound). `LoadPercentage` sits 10-35 points over `PercentProcessorTime`, most near idle where the 30% rule decides, and in 2 of 3 runs read higher at k=0 than at k=2. Since `PercentProcessorTime` is the kernel's total busy time, a reading above it is no real load. `PercentProcessorPerformance` stayed 105-108%, so turbo scaling (my first guess, in a 306 Log line since corrected) does not explain it; the cause inside WMI is not known.
  - Fix in `tools/perf-check.ps1`: the noise input reads `PercentProcessorTime` of `_Total` (same 4 samples over a second, locale-free); a failed read returns `$null`, and `$null` is noisy; any noisy row makes the run NOISY (exit 2), a clean breach still FAIL (exit 1). Thresholds unchanged.
  - Checks: `-SelfTest` ok with five new asserts; two mutations (unread load clean, noisy row passes) each make it fail on the matching assert.
  - Not covered: GPU, disk and memory contention (neither counter sees them); the probe's own WMI reads add load to every column alike. A free-slot perf-check rerun waits for the review.
- 2026-11-18 claude: Review of #521 at 5aa7712b: REQUEST CHANGES, three findings, all confirmed and fixed.
  1. A missing counter value became a clean 0 (`[double]$null` is 0): no instance, no property, null or empty value gave PASS; negative and NaN were clean too. Now `Read-CpuLoadPercent` checks every raw sample before converting it (`ConvertTo-CpuPercent`: a finite number from 0 to 100, else `$null`), and any one bad sample makes the whole read unknown, which is noisy. `Test-NoisyRow` goes through the same check. The reader takes a fake sample source (`-ReadSample`, `-SleepMs`) so `-SelfTest` now runs it end to end on 11 unknown cases, two thrown reads and 5 valid ones.
  2. The probe's k=0 phase started one worker. Now exactly k (`for`), and k is checked from 0 to half the cores.
  3. A failure while sampling or starting workers released the perf lock with workers still running. Now every worker is tracked when it starts and stopped and waited for before the lock is released, also on errors and Ctrl+C. Each sample records alive workers at start and end and its elapsed time. A phase where any worker died prints INVALID, and `-RawCsv` saves every sample.
  - Checks, all offline and light: `tools/perf-check.ps1 -SelfTest` ok. `runs/310-perf-noise-reads-real-cpu-load/mutation-check.ps1`: 5 of 5 mutants fail -SelfTest (unknown load clean, noisy row passes, raw cast to double, no range or NaN check, missing sample skipped). `probe-selftest.ps1` with fake workers, counters and lock: exact k, cleanup before release on a sampling failure and on a start failure, INVALID on a dead worker; both mutants (old `1..k` enumeration, no cleanup in finally) caught.
  - `probe-2026-11-17.txt` is marked superseded: its k=0 rows had one worker and no liveness data. The claim that `LoadPercentage` overstates the load is not established until the fixed probe runs in a free slot, after the re-review. Thresholds unchanged.
