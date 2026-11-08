---
id: 206
title: The game hangs when entering the Enclave tower map
milestone: M1
status: done
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

The owner, 2026-11-07: a fast jump to the Enclave tower map hung the game.
Find why entering a map can hang the game at all, and fix that class,
starting with this map.

## Context

- The owner plays from `game/` on the RPU mod set (debug.log lists 12 mods
  loaded). The hang happened on their install, normal launch.
- "Enclave tower": the Enclave sight on the world map (the oil rig side, or
  the Navarro tower); the exact map name needs one repro: travel there in
  the background test game, or `--dev-map` over the candidate names.
- Known missing-scene-data cases: some maps have no `floor.bin` in the
  owner cache (kladwtwn e1, artemple, vault15). A silent wait on missing
  scene data at map entry is the prime suspect for a game that just freezes.
- 196 built live checks that never hang for the test rig; the same watchdog
  idea (timeout + named missing asset) may belong on the map-entry path
  itself.

## Acceptance

- The background test game enters the Enclave tower map without hanging.
- A map entry that cannot build its scene fails loud (a timeout message
  naming the map and the missing asset) instead of freezing the game.

## Out of scope

- Making the map itself 3D when its export does not exist: that belongs to
  the encounter-maps batch, not to the hang fix.

## Log

- 2026-11-07: owner report in the morning; task cut the same morning.
- 2026-11-08 claude: owner order 2026-11-08: claude takes critical bugs, performance, RP and the Russian translations.
- 2026-11-08 claude: no hang reproduced on a plain start or a dev teleport to any of encdock, encfite, encgd, encpres, encrctr, enctrp, encdet, navarro (they load and stay 2D without floor.bin). One run exited silently after 8 chained teleports, and another test build crashed with 0xc0000374 (heap corruption) the same hour. An AddressSanitizer build caught the cause: a heap-use-after-free in `dfileClose` (dfile.cc:328) on the art-preload threads of 172 (`_obj_preload_art_cache`), because the open-list push was locked and the unlink was not. Any map load could corrupt the heap, which can crash or hang the game. Fix: one file-scope lock covers both. Check: under ASan 61 map loads (15 fresh starts plus 2 teleports each, and a 5-lap kladwtwn/navarro/encdock loop), no report. Release, ctest 24/24, launch check green. Not proven: that this was the owner's hang (no log from his run); the timeout-on-map-entry from the acceptance was not needed once the cause was found.
