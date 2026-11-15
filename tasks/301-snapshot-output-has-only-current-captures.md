---
id: 301
title: Snapshot output distinguishes the current run from older captures
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

Reusing an output folder never makes older devshots look like results of the current attempt.

## Context

`tools/dev-snapshot.ps1:1339-1348` creates an existing `OutputDirectory` and copies captures into it without retiring the previous capture set. Consumers such as `tour.py` and `regression/check.py` scan capture filenames. This is a bounded follow-up to the [snapshot batch](183-owner-feedback-shot.md) and [tour evidence](225-fly-tour-evidence-flight-view-changes.md), and does not change the separate 268-270 capture fixtures.

## Acceptance

- Track and replace only the script's own previous capture filenames and their paired PNGs, preferably with a small scoped filename list. Preserve unrelated user files, reports and other artifacts.
- On a failed attempt, prior captures cannot be mistaken for the current successful result. Use a clear failed/incomplete state that existing callers can recognize, with nonzero exit; do not silently return old filenames.
- A small harness self-test simulates two successful runs with different capture counts and a failed third run in one folder. The current set is exact, unrelated files survive, and failure is observable.
- Existing snapshot callers and tour/regression tool tests pass. Record what filenames the script owns and how failure is exposed.

## Out of scope

Deleting entire user output folders, a general run-receipt framework, changing engine screenshot format or fixing dialogue/white-ground causes.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
