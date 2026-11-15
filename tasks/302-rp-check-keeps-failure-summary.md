---
id: 302
title: The RP boot check writes a complete pass or failure summary
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

An unattended RP check keeps a machine-readable result for every requested map, including failures and timeouts.

## Context

`tools/rp-check.ps1:55-59` throws on the first failed map, before writing `result.json` at line 80. Its successful rows copy `exe_sha256` from the sidecar without comparing it to the candidate. This follows the [RP boot-check batch](188-rp-boot-check.md) and supports [186](186-rp-sfall-gaps-fill.md); it does not implement missing sfall behavior.

## Acceptance

- After preflight, attempt the requested maps independently and always write `result.json` with pass/fail/timeout, the error and artifact path for each row. Exit nonzero if any row fails; retain the isolated test process rules.
- Compute the copied candidate's hash and require successful sidecars to report that executable as well as the requested map. Preserve the existing classic boot/load purpose.
- Use a fake snapshot invocation in a focused self-test: pass, failure, timeout and wrong candidate hash. One failed map does not omit later rows; all failure cases make the aggregate fail. Run that self-test in CI.
- Record the local invocation for a real scratch RP copy when available; if unavailable, state that the synthetic harness check does not prove RP playability.

## Out of scope

Installing RP, changing sfall or scripts, a whole-game health table (271), and scene/visual acceptance for RP maps.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
