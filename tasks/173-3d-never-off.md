---
id: 173
title: 3D never turns off on a local map; what cannot be built stands on a plane
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

On every local map the 3D view stays on. Nothing switches a map, a room or
an object back to the 2D picture; an object the exporter cannot build in 3D
yet stands as a flat picture instead: a billboard turned to the camera, or
its art on a fixed plane.

## Context

- Linked: 193 part B puts a fading black backdrop round the 2D screens (dialogue,
  the world map, the Pip-Boy, the rest) that stay 2D on a local map.
- Folded in (2026-11-04 inbox sweep), the inbox line of 2026-10-27: the
  list of unique objects that need their own 3D model (the big stone head in
  Arroyo, the Vault City gate, the Reno signs, the vertibirds, other one-off
  landmarks, vehicles and large signs). Each stands as a billboard or plane
  here first; name each with its map and shot, ordered by how much a player
  sees it, so the remodelling can follow.
- The owner, 2026-11-04: the world map, dialogs and menus stay 2D.
  **Replaced 2026-11-10:** the owner sees 3D switch off when the inventory,
  the options menu and the other screens open over a local map, and wants it
  never to switch off there: those screens draw over the live 3D view.- Folded in (2026-11-05, from 169): the upper elevations of some desert
  and coast encounter maps (desert5 e1, desert6 e1, shots in
  runs/169-encdesertcoast-in-3d/finds/) export with no floor art and no
  sprites (walls.bin 24 bytes, tiles.png empty, indoor=true), so the 3D
  view is black where the game's own picture shows dim content (2D mean
  46 vs 3D 0.8). The scene ships; the elevation needs its tiles and
  light built or a plane fallback.

- Find every place the 3D view falls back to 2D or switches itself off on a
  local map: a missing or failed cache, an export error, an unsupported
  object or tile, a feature's "switches itself off if it fails to start"
  path that takes the whole view with it, map transitions (147). List them
  in the Log first.
- A map without a cache builds one on entry (or shows its 2D art on the
  ground plane while it builds), never the 2D view.
- An unsupported object, by its kind (the owner, 2026-11-04):
  - a bulky unique object (the crashed vertibird lying on the ground) is a
    billboard turned to the camera, always;
  - a flat thing (a mattress, a rug) is a plane laid the right way and fixed
    in the world.
- Fences have their own working algorithm: do not touch them.

## Acceptance

- The Log lists every fallback found and what replaced it.
- A scripted walk through a map of each finished area (Arroyo, Klamath, Den,
  Modoc, Vault City, Gecko, Broken Hills, New Reno) stays in 3D the whole
  way; the game log shows no fallback line.
- ctest and the regression pass.

## Out of scope

The world map, dialogs, menus.

## Log

- 2026-11-04 audit: `scene_view::syncToMap` gave up after any floor, wall or
  projection load error, cleared the loaded map, and logged `stays 2D`.
  Changed this path to retry the floor sidecar and keep its 3D ground plane
  when walls/projection are missing or invalid. `activeView()` now reports the
  actually loaded level (`Floor`) so presentation draws the ground-plane view.
  A missing/invalid floor sidecar still has no ground geometry to show; this
  remains an unresolved fallback and needs an on-entry cache build or a
  ground-plane source from the live map. No claim that this case is fixed.
- Other inspected fallback paths: launcher reports 2D if the exporter is
  missing or export fails; `loadScene` rejects missing floor/walls/projection;
  map transition retries per map/elevation; unsupported object cards are
  skipped when rotation, FRM lock/data/dimensions or renderer insertion fails;
  missing/hidden hero only skips the hero card; explicit 3D-off setting,
  menus/dialogs/world map, movies and iso-disabled fades are deliberate gates;
  OpenGL startup failure uses the vanilla SDL renderer. The object-card and
  hero skips currently have counters/logs but no replacement for every skip;
  inventory and representative area walks remain outstanding.
- 2026-11-04 full regression completed: all 88 references matched (`0 changed
  pixels`). First combined 16-map runtime tour proved 9 stops in full 3D, then
  caught the game during map-transition state for 7 remaining snapshots.
  Independent fresh-game captures for the 8 requested areas all reported
  `scene_view=full`, `preview_availability=available`, and zero skipped object
  cards. Two follow-up area-tour attempts failed in the harness: Arroyo stopped
  at `arvill2` because it is absent from the worldmap registry, and Klamath
  reached `klacanyn` with 3D available but produced no third snapshot; the
  helper waited for its fixed count until timeout. Only those test processes
  were stopped. The attempted tour-readiness code change was reverted. Do not
  claim scripted area-walk acceptance; next diagnostic should make the helper
  stop promptly and report the actual route/availability instead of waiting
  for an impossible snapshot count. The eight independent map captures and
  full regression remain valid.

- 2026-11-08 glm: the incomplete-map fallback landed as PR #272 (e274f59b): a map whose walls or projection sidecar cannot load keeps its 3D ground plane; activeView() reports the loaded level. Still owed: the missing-floor path (no floor data cannot be replaced) and the scripted tour acceptance.
- 2026-11-10, glm. The scripted area walk now runs clean and closes that
  acceptance item: one background process, startup Klamath then
  arvillag → denbus1 → modmain → vctyctyd → gecksetl → broken1 → newr1
  (Arroyo via arvillag; arvill2 is absent from the worldmap registry), with
  -SkipScriptBlocking so map-entry scripts do not hold the state. Every stop
  reports scene_view=full, 0 skipped object cards, and the captured console
  has 0 "stays 2D" lines - the 2026-11-04 harness hangs are gone (the tour
  engine now snapshots where the game is and moves on when a stop cannot be
  reached). Evidence: runs/173-3d-never-off/area-tour/ (8 shots, 8 sidecars,
  console log).
- 2026-11-10, glm. The incomplete-elevation family (169's find) is alive in
  a new form: desert5 e1 with the 1.13.0 re-export has floor geometry but
  0 sprites, 0 walls, an empty tile atlas - classic 2D shows a lit desert
  strip with two tents, barrels and a tree, the 3D view shows a dark plane
  and the hero (runs/173-3d-never-off/desert5-e1/ vs
  desert5-e1-classic/scr00000.png). The view stays 3D (no 2D fallback), but
  the elevation's content is lost in the export walk. That is an exporter
  content bug of its own, not a fallback-ladder gap; ask the owner whether
  to open a dedicated task (the 224 sweep will hit the same family).
- 2026-11-10, glm. Owner question on the last unresolved fallback: a map
  with no cache at all (kladwtwn e1 has no cache directory; the 211 run
  showed vctyctyd refusing the toggle with "у этой карты нет 3D-сцены" and
  staying 2D, runs/211-3d-mode-toggle-no-response/sceneless-map-refusal.jpg)
  still lands in 2D on entry. "Builds one on entry" cannot run the exporter
  inside the shipped game - the exporter is Python tooling. Options: (a) the
  launcher pre-builds every local map's cache; (b) a renderer fallback that
  lays the live map's own 2D floor art on a 3D ground plane until a cache
  exists (a feature of its own size); (c) accept 2D on cacheless maps.
  Recorded here, moving on without waiting.
- 2026-11-10, glm. The folded-in list of unique objects that need their own
  model (the Arroyo stone head, the Vault City gate, the Reno signs, the
  vertibirds): producing it map by map with shots is exactly what the 224
  geometry sweep and the city walk do; named finds go to those logs and the
  consolidated list lands there. This task keeps the fallback ladder and
  the never-2D guarantee.
- 2026-11-10 claude: the owner's ask folded in: 3D stays on under the inventory, menus and other screens opened on a local map.
