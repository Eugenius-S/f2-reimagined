---
id: 293
title: The existing cache check rejects missing or damaged scene artifacts
milestone: M1
status: doing
owner: claude
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
- 2026-11-18 claude: Fix in PR #523, waiting for an independent review. Not a fix or a cause for 270/306's white floor.
  - Shown first: 13 synthetic cases in `test_export.CacheCheckTests` against the old code; every damaged cache passed with exit 0 ("scenes present"): a missing upper elevation (the old check took any `*/scene.json`), changed bytes (the old `stale_reason` checked only that the listed files exist).
  - Fix, one shared rule: `--check` now runs the same per-elevation `stale_reason` an export run uses, over the elevations the map loader reads from each map header (no e0 assumed). `stale_reason` now also requires the directory to hold exactly the files its provenance's `artifacts_sha256` lists, `scene.json` and every declared sidecar in that table, and each file to hash to its recorded value; a read error is a reason, never current. Any problem prints `not current: <map> e<elevation>: <reason>` and exits 3; a map that does not load prints `FAILED` and exits 3; a complete current selection prints `scenes present: ...` and exits 0, as before. `check_scenes` is gone (no other caller). The launcher's path is unchanged: any non-zero check runs the export.
  - Fixtures (synthetic maps written by `MapFile`, a DAT2 game root, no game assets): map `two` e0+e1, map `upper` e1+e2 with no e0; valid set, missing elevation, broken `scene.json`, broken `provenance.json`, missing and truncated PNG, unreadable file, changed `walls.bin`, empty and incomplete artifacts table, table without `scene.json`/sidecar with the files gone, changed map input; one CLI case runs `export.py` as a program for exit 0 and 3. Two older fixtures wrote fake artifact hashes and now write real ones.
  - Checks, light only: `py -3 -m unittest discover -s tools/scene_export -p "test*.py"`: 160 ok (157 in test_export). A mutation run of `CacheCheckTests` over a temporary copy: 5 of 5 mutants caught (no hash compare, no extra-file check, no required-file check, check passes stale scenes, read error is success). `git diff --check` clean, `tools/check_public_content.py origin/main` clean.
  - Not run (heavy slot held by GLM): a real export, `--check` over a real `all,rp` cache and its time. The check now replays each scene's recorded reads and hashes its files, the same work as a no-op export run, so it takes longer than the old glob; to be measured on the real cache before merge.
