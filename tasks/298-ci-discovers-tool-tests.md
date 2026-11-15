---
id: 298
title: CI discovers the current top-level Python tool tests
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A new top-level `tools/test_*.py` suite runs in CI without adding another hand-written workflow command.

## Context

`.github/workflows/verify.yml:205-218` discovers tests in exporter, shapes and regression folders, but lists top-level tools suites one by one. This is a follow-up to the [CI test coverage batch](252-ci-runs-all-its-tests.md); it is a maintenance gap, not evidence that today's listed tests are skipped.

## Acceptance

- Replace the individual top-level unittest commands with one discovery command for `tools/test_*.py`, preserving the existing dependencies and the separate subdirectory suites.
- Show the discovered test module names in CI or the Log and compare them with the current tracked top-level test files; all are included.
- A temporary uncommitted failing top-level test causes the same discovery command to fail, then is removed. The full existing tool test sets pass.

## Out of scope

New CI frameworks, a recursive test runner, moving test files, or changing game test coverage.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
