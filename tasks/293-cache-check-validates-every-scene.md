---
id: 293
title: The existing cache check rejects missing or damaged scene artifacts
milestone: M1
status: todo
owner: any
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`export.py --maps <selection> --check` succeeds only when every actual elevation in the selection has its complete, current cache.

## Context

`tools/scene_export/export.py:450-455` currently accepts a map when any elevation contains `scene.json`. `stale_reason` at lines 521-546 checks whether the recorded artifact names exist, without comparing their recorded hashes. A missing upper floor or modified `tiles.png` is therefore outside the existing check. This is a bounded follow-up to the [player cache batch](220-launcher-builds-every-map.md) and [missing-cache task](273-missing-cache-builds-on-entry.md), and supports [270](270-white-ground-capture.md); it does not establish the cause of 270's white floor.

## Acceptance

- Reuse `--check`, the map loader's actual elevation list and existing provenance fields. Missing elevation, missing required file, changed artifact bytes, malformed scene/provenance JSON, unreadable PNG and outdated inputs return a nonzero exit with map/elevation and reason; an unchanged complete selection returns zero.
- Synthetic fixtures cover a map with two elevations, a map with e1/e2 but no e0, malformed JSON, a missing or truncated PNG, a changed binary sidecar and a valid cache. Do not regard an empty or incomplete artifacts table as complete.
- The launcher's existing check path still triggers export on a nonzero check result. Exporter unit tests pass; record the commands and results.

## Out of scope

Changing renderer behavior, cache publication (294), new diagnostic services, or identifying the root cause of 270. Keep existing human-readable output unless a real caller requires more.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
