---
id: 300
title: Task metadata and dependency mistakes fail a small local check
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

Validate task headers and dependency references before a board update can silently accept malformed entries.

## Context

`tools/tasks.ps1:82-90` parses headers permissively; selection at lines 213-226 treats dependency strings as IDs without reporting missing references or cycles. The [board truth batch](248-board-tells-the-truth.md) and [lock selection batch](249-tasks-script-live-and-blocked.md) depend on consistent metadata. This is a validation gap, not a claim of an existing broken dependency.

## Acceptance

- Add a small local check, usable in CI, for unique numeric IDs matching filenames, required header fields and supported enum values including `blocked`, tier 0-4, existing dependency IDs, self-dependencies and cycles.
- Run it against current tasks; fix only metadata errors it actually reports, preserving owner priorities and task meaning. If an ambiguous historical entry exists, name it instead of inventing a dependency.
- Fixtures cover valid headers, duplicate ID, unknown dependency, cycle and legal blocked status. The check and `tools/tasks.ps1 -SelfTest` pass; CI runs the metadata check.

## Out of scope

New task statuses, assignment changes, rewriting task bodies, a YAML framework, or changing Polka's synchronization.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
