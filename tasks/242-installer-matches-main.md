---
id: 242
title: The installer gives a player exactly the current main, wide art included, and rebuilds only changed maps
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

A player who installs or updates through the launcher gets what main is
today: the current build, the 21:9 menu wallpaper and loading screens the
owner's dev game shows, and the iron Vault-Tec bottom panel. An update
rebuilds the 3D scenes of only the maps whose inputs changed, not every map.

Owner order 2026-11-09: critical, next right after the critical bug fixes.

## Context

- Owner find 2026-11-08: the GitHub install (Steam folder, launcher) shows
  the 4:3 menu wallpaper; the owner's dev game shows 21:9. Found then: the
  wide art lives only in the owner's dev game folder. Main menu:
  `ce/src/mainmenu.cc:316-326` loads
  `game/user-assets/fullscreen-20261028/wallpapers-selected/hires/menu/*21-9.png`
  and `hr_mainmenu.png` (git-ignored owner assets); loading screens:
  `data/art/splash/splash0-6.rix` and `data/art/russian/splash/splash0-6.rix`
  (551,338 bytes each), absent from the install.
- Keep the default iron bottom panel with the Vault-Tec logo at the edges;
  the panel with brown leather straps is not wanted (inbox 2026-11-09).
- The Steam install on 2026-11-08 ran e952c76c9 while main was a35409ae3:
  check how the launcher picks the build and that it always takes the newest
  main build.
- The launcher builds every map's scenes (220) with real progress (232);
  today an update rebuilds them all.
- 128 (gpt, blocked) holds the licenses side of the wide art; the art the
  owner already uses is the one to ship.

## Acceptance

- A clean install and an update through the launcher end on the same commit
  as main's newest build; the launcher says which.
- The installed game shows the 21:9 wallpaper and the wide loading screens
  in English and Russian, and the iron Vault-Tec panel.
- A second update with no map-relevant change rebuilds no map; a change to
  the exporter or to one map's inputs rebuilds what it touches. The log names
  the rebuilt maps and why.
- Nothing from the owner's game files goes into the repo; ctest passes.

## Out of scope

New art; the Russian translations list (227); the RP option (186).

## Log

- 2026-11-09 claude: cut from the owner's order; the 128 find and the panel choice folded in.
- 2026-11-09 claude: wide menu, death and loading art ship in the release (PR 380, 9a08d6386): fixed release `wide-art` holds the owner's art, the build job packs it, the launcher copies the folders and pulls once more where they are missing. Left: changed-maps-only rebuild, the launcher naming the build.
- 2026-11-09 claude: done. PR 382 (563ce081d): the freshness key counts game data only through the files each scene read or looked for; the game rewriting fallout2.cfg or the launcher adding the Russian mod rebuilds nothing; the log names each rebuilt map and why; the launcher names the installed build at the end. Existing installs rebuild every scene once after this ships.
