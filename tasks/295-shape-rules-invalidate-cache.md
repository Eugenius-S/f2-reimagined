---
id: 295
title: Changes to JSON shape rules invalidate the affected export cache
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

The export fingerprint includes the JSON shape rules that the exporter actually reads.

## Context

`tools/scene_export/export.py:26-28` loads `shapes/fixed_cards.json`; `tools/scene_export/vendor/fp/scene.py:63` loads `shapes/overrides.json`. `static_inputs` in `export.py:497-505` hashes Python sources and a fixed adapter list, not those JSON files. This follows the [furniture and props batch](191-batch-furniture-props.md) and [242's cache invalidation work](242-installer-matches-main.md).

## Acceptance

- Add those two existing rule files to the shared fingerprint inputs and provenance using the current hashing pattern. A prior cache is stale after either rule file changes.
- A fixture changes each rule file independently and observes a fingerprint change; changing an unrelated documentation file does not change it.
- Source and standalone exporter retain access to the same rule files. Exporter tests pass; record the exact checks.

## Out of scope

Changing shape rules, inventing per-object dependency tracking, cache publication (294), or changing any binary scene format.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
