---
id: 220
title: The launcher builds the 3D scenes of every map, not only Klamath
milestone: M1
status: doing
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A player who installs with the launcher gets every map in 3D. Today the
launcher's default `--maps` is `klamath` (`ce/src/launcher_main.cc`,
`kDefaultMaps`), so an installed game shows the other 151 maps in 2D,
although the exporter covers all 152 since 170 and 171.

## Done when

- The exporter knows a group `all`: every map of every group, each once.
- The launcher's default is `all`; `--maps` still narrows it.
- `--check` with `all` answers 0 only when every base scene exists, so an
  install that has only Klamath gets the rest on its next start.
- Exporter tests cover `all`; ctest passes.

## Log

- 2026-11-08 claude: cut in the project audit.
