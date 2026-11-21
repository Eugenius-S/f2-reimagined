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
- 2026-11-18 claude: Review fix (rereview-3 of PR #523). P1: the required set came from the provenance, so dropping `tiles.png` or `walls.bin` together with its table entry (and its `wall_sidecar` declaration) still checked current. P2: a file rewritten together with its recorded hash (`scene.json` as `{`, `tiles.png` as plain bytes) checked current.
  - Shown first: 4 new cases fail on 7f02121f, each one exits 0 "scenes present".
  - Fix: the required set no longer comes from the manifest. `REQUIRED_OUTPUT_FILES` holds what every export writes (`scene.json`, `tiles.png`, `light.png`, `floor.bin`, `walls.bin`, `baked.bin`, `projection.bin`, `shadows.bin`); `scene_references` adds what the scene itself names: its atlas pages, `tiles`/`light`/`rock` images, the `sprites<n>.png` page of every `walls.bin` batch (headers read, as `walls.cc` opens them) and `roof_fringe.png` when `floor.bin` holds fringe or patches. Formats: `scene.json` must parse and name this map and elevation, `walls.bin` and `floor.bin` must decode their headers, every `.png` must be a PNG that passes `Image.verify` (chunks and CRCs to IEND).
  - Fixtures: one shared `write_scene_artifacts` writes a tiny but valid file set.
  - New cases: a contract file dropped with its table entry (`tiles.png`, `walls.bin` with its declaration, `light.png`, `shadows.bin`); a scene-named file dropped (`sprites0.png`, wall page `sprites1.png`, `roof_fringe.png`); a paired rewrite of `scene.json` (`{`, another elevation's scene), `tiles.png` (not a PNG), `light.png` (cut before IEND), `sprites1.png` (a GIF), `walls.bin` and `floor.bin` (a bare magic).
  - Checks: 165 ok. Mutations: 12 of 12 caught (the 5 above plus: contract set from the manifest, scene references unchecked, wall pages ignored, fringe ignored, another scene's `scene.json`, no PNG verify, any image format).
  - Limits: `baked.bin`, `projection.bin` and `shadows.bin` are checked by hash only; PNG pixels are not decoded. The real `all,rp` check and its time are still pending (now also a header walk and a PNG verify per scene).
