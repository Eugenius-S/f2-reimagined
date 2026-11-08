---
id: 186
title: The Restoration Project plays in CE, in 3D, and the installer offers it
milestone: M1
status: todo
owner: any
suggested: any
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
