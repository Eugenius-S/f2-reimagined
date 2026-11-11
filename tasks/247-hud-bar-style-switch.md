---
id: 247
title: The options menu switches the bottom HUD bar between Original and RP (the belt)
milestone: M1
status: todo
owner: any
suggested: any
priority: medium
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The owner's ask (2026-11-10): the options menu gets a switch for the look
of the bottom interface bar: **Original** (the vanilla bar) or **RP** (the
Restoration Project's bar with the belt). The choice applies in play and is
kept in the config.

## Context

- The options menu and its 3D panel: tasks 017, 112, 130, 133.
- RP is installed by the launcher (186 part 3); its bar art ships in
  `mods/rpu.dat`. Without RP on disk the switch shows Original only (or is
  disabled with a reason).
- The wide HUD bar art (128) is a separate question; keep both working.

## Acceptance

- The options menu has the switch with both choices, in English and Russian.
- Each choice draws its bar in play, in 3D and in classic mode; the choice
  survives a restart.
- Without RP on disk the game still plays and the switch says why RP is not
  available.
- ctest passes; the regression stays green.

## Log

- 2026-11-10 claude: cut from the owner's ask in chat.
