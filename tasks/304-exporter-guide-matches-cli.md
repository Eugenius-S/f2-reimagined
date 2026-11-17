---
id: 304
title: The exporter guide describes the current all-map and RP workflow
milestone: M1
status: doing
owner: claude
suggested: glm
priority: low
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The exporter README gives working current commands and states the supported source-priority and cache behavior accurately.

## Context

`tools/scene_export/README.md:3` still describes a one-map adapter restricted to `kladwtwn` e0 and refusal of active mods; `export.py` supports map groups, all actual elevations, `rp` and active mod sources. Lines 21 and 31 also describe old preview/vendor limitations as current. This belongs to the [player export batch](220-launcher-builds-every-map.md) and [RP content batch](187-rp-content-in-3d.md).

## Acceptance

- Update the existing guide from actual CLI/source behavior: one map/elevation, named groups, `all`, separate `rp`, `--cache-root`, `--check`, incremental export, source priority and read-only game inputs.
- Clearly distinguish current commands from historical implementation notes. Preserve licenses and donor revision attribution; do not call modified vendored files verbatim.
- Check every documented switch against `py -3 tools/scene_export/export.py --help` and `--maps` parsing tests. Exporter tests pass; docs have no personal paths or long dash.

## Out of scope

Code changes, changing supported maps or formats, claiming new export/check guarantees before their task merges, or rewriting unrelated documentation.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
- 2026-11-18 claude: Taken. Branch `task/304-exporter-guide-matches-cli`, stacked on 295 (`5fffaae7`), on 294 and 293; none accepted yet, so the guide describes those branches' behavior. Documentation and `--help` only; no export, build or game run.
- 2026-11-18 claude: Guide rewritten in PR #527 (draft, stacked on #525), describing the 293-295 branch behavior (not accepted yet).
  - Current commands, from the code and `--help`: one map/elevation (always built), `--maps` with groups, `all` (without `rp`) and `all,rp`, `--project-root`/`--game-root`, `--cache-root` with `F2CE_SCENE_CACHE`, `--check` (exit 0/3, read-only), `--force`, the output lines and exit codes, the test command.
  - Source priority as `configured_sources` builds it, active mods included (the old guide said they were refused); the current-scene rule; the staging publish; recovery with safe manual steps that keep the last backup (move it out of the cache, never delete it); verified versus not verified; the standalone build; a table of scene files with current versions (floor v7, walls v5, baked v7, projection v1, shadows v1) pointing at the modules for layouts.
  - Historical notes moved to a short History section; the old per-version format prose, the 1,259-wall-row count and the dev hero preview details were dropped (stale; the modules hold the formats). Donor paragraph fixed: the vendored files are a modified subset, not verbatim; revision and licenses kept.
  - Checks, light: a script compared every switch in the guide with `export.py --help` (9 of 9 present), the group list with `MAP_GROUPS` plus `rp` (18, same order), `all` holds no RP map, the `--map`/`--elevation` defaults; `test_export.TargetTests` OK. No long dash; `git diff --check` and `tools/check_public_content.py origin/main` clean. No export, build or game run.
  - Seen, not fixed here (out of scope): the JSON summary `export_scene` prints names the `.staging` folder as `output`, because it runs before the move into place.
