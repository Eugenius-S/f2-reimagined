---
id: 304
title: The exporter guide describes the current all-map and RP workflow
milestone: M1
status: todo
owner: any
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
