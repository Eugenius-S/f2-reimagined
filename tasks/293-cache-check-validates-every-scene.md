---
id: 293
title: The existing cache check rejects missing or damaged scene artifacts
milestone: M1
status: done
owner: gpt
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

### 2026-11-23 Windows candidate

Reused the draft from [PR #523](https://github.com/Eugenius-S/f2-reimagined/pull/523)
on current main, retaining floor-receiver output support. Reject unsupported
manifest names before artifact access, reject links and reparse points in
cache paths, and continue reporting other elevations after an invalid path.

- Exporter suite: `py -3 -m unittest discover -s tools/scene_export -p 'test*.py'`,
  177 passed. Includes authored e1 and e2 without e0 and malformed cache fixtures.
- Standalone CLI on an isolated stock Windows game root: three maps
  (`arcaves,arvillag,denbus1`), six authored scenes. Complete cache returned
  0 in 1.09 s. With the ARCAVES e2 cache withheld and DENBUS1 e0 tiles.png
  truncated, check returned 3 and named both failures. The ordinary exporter
  rebuilt exactly those two scenes in 15.97 s, kept four current, then check
  returned 0 in 1.07 s and a repeat export rebuilt nothing.
- Read-only coverage on a warm existing cache: 111 maps, 162 authored scenes;
  detected the changed exporter in all 162, without rewriting any scene
  (12.91 s first check; 6.90 s in a second profiled run excluding startup).
  This is stock test data, not a full Restoration Project check. An exporter
  update intentionally invalidates existing cached input fingerprints, so the
  first update can rebuild the scenes. The launcher still asks the exporter
  to repair on any nonzero check status.

### Released

Windows latest: `1c893f5af12230d72a73272f6844050c0b76f31b`. Clean Windows
Release and CTest, all 177 exporter tests, and native 3D on the repaired
ARCAVES e2 from the exact merged build passed. The release workflow finished
successfully, and the public stamp, ZIP and installer were checked.
Restoration Project was not run in full. The initial update rebuilds stale
scene caches.
