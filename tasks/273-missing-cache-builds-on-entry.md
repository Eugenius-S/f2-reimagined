---
id: 273
title: A map with no scene cache still opens in 3D
milestone: M1
status: doing
owner: glm
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Entering a map whose scene cache is missing or stale never shows the 2D view: the cache is built on entry, and the map's 2D art lies on the ground plane while it builds.

## Context

- A piece of 173 (3D never turns off), cut out to be checkable on its own. First record what main does today: run the test game with an empty `-SceneCache` folder on kladwtwn and on one map outside the launcher's set, and quote the log.
- The export is `tools/scene_export/export.py`; the launcher builds caches for players (220).

## Acceptance

- With an empty cache folder, kladwtwn and two other maps open in 3D; the log has no fallback line; frames in `runs/273-missing-cache-builds-on-entry/` show the waiting state and the built scene.
- A cache from an older exporter version is rebuilt the same way, not loaded.
- The wait is stated in the Log in seconds per map; ctest passes.

## Out of scope

Making the export itself faster.

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-19 glm: entering a map whose cache is missing, stale or damaged now
  starts the shipped scene-exporter.exe for that map on entry
  (`ce/src/scene/scene_build.{h,cc}`); provenance.json's `exporter_version`
  decides (anything unknown reads stale: fail-closed), a cache whose
  provenance matches but whose files do not load is rebuilt with `--force`,
  and a finished build clears the failure so the scene loads in the same
  frame. With no exporter beside the exe the honest stays-2D line stays.
  `kExpectedExporterVersion` is pinned to export.py's `EXPORTER_VERSION` by
  `tools/test_exporter_version_sync.py`; new suite `ce/tests/scene_build` in
  the scene matrix. Check: full Release build + ctest 29/29 locally.
  Next: the waiting state on the 3D ground plane (the map's 2D art while the
  build runs) and the runtime frames in `runs/273-missing-cache-builds-on-entry/`
  need a game-run slot; the wait in seconds per map goes here then.
