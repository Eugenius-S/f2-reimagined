---
id: 247
title: The options menu switches the bottom HUD bar between Original and RP (the belt)
milestone: M1
status: blocked
owner: gpt
suggested: any
priority: high
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


### 2026-11-11 gpt
Done: No implementation was made. The existing `ui.iface_bar_mode` key is a screen geometry toggle: `svga.cc` uses it for the visible height, `game_mouse.cc` uses it for the click region, and `interface.cc` uses it to suppress side panels. The bar background is selected during `interfaceInit` in `ce/src/interface.cc`, where the vanilla `FrmImage` is blitted or the custom width art is loaded. The requested RP art selection and the missing-RP availability explanation therefore require changes outside the authorized files. The 3D options panel currently exposes only `3d` settings, so adding a UI setting there also does not cover the classic options menu within the allowed boundary.
Next: Re-scope the task to authorize the interface art-loading path and the classic preferences path, then implement, test, and capture both bar choices.
Check: Inspected `ce/src/settings.cc`, `ce/src/settings.h`, `ce/src/scene/options_panel.cc`, `ce/src/interface.cc`, `ce/src/svga.cc`, and `ce/src/game_mouse.cc`. No build, ctest, regression, screenshot, persistence, or RP-presence checks were run because no implementation was possible within the file boundary.

### 2026-11-11 gpt
Done: Implementation remains blocked because the installed RP archive has no bottom-bar FRM. The original `art/intrface/iface.frm` in `master.dat` is 640x99 (one frame). `mods/rpu.dat` has no `art/intrface/iface.frm` and no corresponding 640x99 interface FRM; its 18 interface FRMs are unrelated screens or dialogue art. No code was changed.
Next: Supply the intended RP bottom-bar FRM in `mods/rpu.dat`, then implement and verify the selector.
Check: Read-only scan of `master.dat` and `mods/rpu.dat` with `tools/scene_export/vendor/f2lib/dat2.py` and `f2lib/frm.py`; original decoded as 640x99, RP counterpart absent.
