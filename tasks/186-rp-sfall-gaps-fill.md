---
id: 186
title: The Restoration Project plays in CE, in 3D, and the installer offers it
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

A player ticks "Restoration Project" in the installer and plays RP in CE,
its maps in 3D like the vanilla ones. The next stage after the installer
and optimization (the owner, 2026-11-08). 185 (the sfall gap list) and 187
(RP maps in 3D) are folded here; their Logs keep the history.

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
  Follow sfall's own documented behaviour for each opcode and hook.
- New sfall code lives in CE's sfall modules, not spread through upstream
  files (AGENTS.md).
- Where it stands (2026-11-08): 184 installs RP into a scratch copy and it
  boots; 188 checks it still boots. 185's inventory (#284) stopped at the
  EPAMAIN1 map-entry script that never reaches a presentable scene. 187
  (#289) exports RP maps; SFSHENG elevation 1 fails to start on the after
  side. The installer's RP option (199) is shown disabled, "coming soon".

## Acceptance

- The gap list is complete: one line per sfall opcode, hook or ini key RP
  needs and CE lacks, with its count, where it fired, and whether it blocks
  play. EPAMAIN1 reaches a presentable scene.
- Every blocking gap is implemented, then the rest by how often they fire;
  the Log shows the gap count before and after.
- Every RP map exports without errors, SFSHENG elevation 1 included; RP's
  restored areas have fly tour spots in a separate spots file run only when
  an RP game root is given (RP is not in CI). A walk through two restored
  areas stays in 3D.
- The installer's RP option is enabled: it fetches RP from its official
  release at the player's request (never rehosted), installs it and builds
  its 3D scenes.
- ctest passes; a vanilla game still plays (the regression passes).

## How

In this order, one PR each, so main gets each piece as it lands: finish the
gap list; the blocking gaps; RP maps in 3D; the installer option; the
remaining gaps.

## Out of scope

Russian translations (227).

## Log

- 2026-11-08 claude: 185 and 187 folded in at the owner's request; made the
  next stage.
- 2026-11-08 claude: owner order 2026-11-08: claude takes critical bugs, performance, RP and the Russian translations.
- 2026-11-10 claude: parts 1 and 2 landed (#391, which carries #388): the gap
  list with runtime counts is in `runs/186-rp/README.md`, 12 gaps (1 blocking)
  before, 7 (0 blocking) after; `set_fake_perk` and its kin, `fs_seek`,
  `fs_read_short`, `fs_write_short` and an editable `fs_copy`. EPAMAIN1 reaches
  its scene. Left: the RP map exports and fly tour spots, the walk through two
  restored areas, the installer option; not seen live: the `epac3` reply, an
  edited walking FRM drawn in play, fake perks in the character screen.
- 2026-11-10 claude: part 3: the installer's RP option. The launcher fetches
  `rpu_<tag>.zip` from RPU's latest official release on request, applies
  `rpu-install.bat`'s steps (old patch archives and loose `data/` aside, saves,
  music, `data/*.txt|gcd` kept), keeps every replaced file in `backup/rpu`, and
  writes `mods/rpu.dat` last so a cut-short copy is retried. The scene export
  then takes `all,rp` (the exporter's new `rp` group). Checked end to end on a
  fake package and game folder (install, re-run keeps it, failed download
  changes nothing, a failed copy is finished by the next run); not yet run
  with the real 900 MB package. Walk shots: `runs/186-rp/abbey-walk-part3.jpg`
  (3D), `runs/186-rp/sfsheng-walk-part3.jpg` (the dead trees draw near black:
  check against 2D before closing). Left: the real-package install, that
  SFSHENG tree check, and the not-seen-live items above.
- 2026-11-10 claude: the owner's Steam install after the RP install (photo of the main menu in chat): the menu shows no wide art and the button labels run over the buttons. Found: `user-assets/fullscreen-20261028` in that folder is empty (emptied 2026-11-09 23:28, cause not found: a fresh install and an update over an empty folder both fill it in a test), so the menu falls back to HR_MAINMENU.FRM from `mods/f2_res.dat`, and that fallback layout puts the labels over the buttons. To do: the launcher checks the menu PNG itself, not its folder, and puts the art back; the fallback layout keeps the labels clear of the buttons.
- 2026-11-10 claude: the owner: that Steam install went over a game with an older build of ours. The installer must clean what the new build does not need (leftovers of the older build) when it installs or updates over it; the empty wide-art folder came from such an install.
