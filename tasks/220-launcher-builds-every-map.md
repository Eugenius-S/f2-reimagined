---
id: 220
title: The launcher builds the 3D scenes of every map, not only Klamath
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

A player who installs with the launcher gets every map in 3D.

## Context

The launcher's default `--maps` is `klamath` (`ce/src/launcher_main.cc`,
`kDefaultMaps`), so an installed game shows the other 151 maps in 2D,
although the exporter's groups cover all 152 playable maps since 170 and
171. The exporter's `--maps` takes one group or a list of map names, so a
list of groups is read as map names.

## Acceptance

- The exporter knows a group `all`: every map of every group, each once;
  `--maps` takes a comma-separated mix of maps and groups.
- The launcher's default is `all`; `--maps` still narrows it.
- `--check` with `all` answers 0 only when every map has a scene (any
  elevation: newr1a and newr2a have no elevation 0), so an install that has
  only Klamath gets the rest on its next start.
- A repeated export skips every scene whose inputs (exporter code, the game
  data it read, config, archives) are unchanged; `--force` rebuilds.
- Exporter tests cover `all`; ctest passes.
- Both READMEs say every map is in 3D.

## Out of scope

Export speed beyond what an update needs, a map picker in the launcher UI,
RP maps (187).

## Log

- 2026-11-08 claude: cut in the project audit.

### 2026-11-08 claude
Done: `export.py --maps all` (any mix of maps and groups), the launcher's default is `all`. Measured on the demo game: the first full export takes about 23 min (222 scenes, 152 maps, 844 MB); a second run took 22 min again, so the exporter now skips a scene whose provenance `inputs_sha256` still matches a replay of its recorded reads (static hashes once per run, gf caches cleared per scene so the reads are complete and order independent); a repeat run over a complete cache takes 12-15 s, `--check` 5 s. The launcher's `--force-export` passes `--force`. Found on the way: coast11 crashed the exporter (fixed in 226).
Next: nothing.
Check: `py -3 tools/scene_export/export.py --game-root <game> --maps all --cache-root <dir>` twice; the second prints `"current": true` for every scene.

### 2026-11-08 claude
Fix: #344 landed only the first commit; the skip and `--force` commits never reached origin, so a launcher update re-exported every scene (17 min on the player install). This PR brings them back. Check: klamall, kladwtwn, denbus1 exported twice into a fresh cache: 54 s, then 2 s with 5 of 5 scenes current; 143 exporter tests pass.
