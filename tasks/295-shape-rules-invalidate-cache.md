---
id: 295
title: Changes to JSON shape rules invalidate the affected export cache
milestone: M1
status: doing
owner: claude
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
- 2026-11-18 claude: Taken. Branch `task/295-shape-rules-invalidate-cache`, stacked on 294 (`a9e5d201`), which is stacked on 293; neither is accepted yet. Light synthetic checks only while the heavy slot is GLM's.
- 2026-11-18 claude: Fix in PR #525, stacked on #524 and #523; waiting for an independent review.
  - Shown first: the new `test_export.ShapeRulesFingerprintTests` against the old code; a changed `shapes/fixed_cards.json` kept the same fingerprint.
  - Fix: `static_inputs` hashes `shapes/fixed_cards.json` and `shapes/overrides.json` with the adapter files, so both go into `inputs_sha256` and into provenance's `adapter_sha256`. No rule file and no format changed.
  - Same files for source and standalone: `export.py` reads `HERE/shapes/fixed_cards.json`, `vendor/fp/scene.py` reads `shapes/overrides.json` three levels up, which is the same `HERE/shapes` (a test checks the path); `build_standalone.ps1` ships the whole `scene_export` tree as data, `shapes` included (read only, not built).
  - Fixture: a temporary copy of the exporter tree with `export.HERE` pointed at it and no game sources; a README note keeps the fingerprint, each rule file changed alone gives a new one and its hash shows in `adapter_sha256`.
  - Checks: `py -3 -m unittest discover -s tools/scene_export -p "test*.py"`: 178 OK. Mutations of `ShapeRulesFingerprintTests` over a temporary copy, 3 of 3 caught: fixed cards not hashed, overrides not hashed, every file in the tree hashed. `git diff --check` and `tools/check_public_content.py origin/main` clean.
  - Effect: the first export after this lands rebuilds every cached scene once (new fingerprint inputs), as any `export.py` change already does. Not run: a real export or check (heavy slot is GLM's).
