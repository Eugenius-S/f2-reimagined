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

### 2026-11-12 gpt
Done: Found the installed RP interface bar art in `mods/rpu_russian.dat`, member `art/intrface/iface.frm`. It is 640x99, one frame, 63,434 bytes, and differs from `master.dat`'s `art/intrface/iface.frm`. `mods/rpu.dat` itself still has no `iface.frm`. No implementation or game files were changed.
Next: Implement the selector using the installed RP member and verify it in classic and 3D modes.
Check: Read-only scan of every `.dat` in the installed RP `mods/` directory using the repository's DAT2 and FRM readers; checked the loose `data/art/intrface/` location (absent), `mods/mods_order.txt` (the Russian archive follows `rpu.dat`), and `ddraw.ini` interface keys (`ActionPointsBar`, `ExpandBarter`, and other interface options; no bar-art path key). Compared decoded dimensions and SHA-256 for the base and RP `iface.frm` members. Rendered the RP FRM with the installed game palette to an ignored `build/` preview and visually confirmed it is the complete bottom interface bar with belt slots. Build, ctest, regression, and runtime checks were not run.

### 2026-11-12 gpt
Done: Added the Original/RP setting to the 3D options panel and config. Style art loads directly from `master.dat`, `mods/f2_res.dat`, and `mods/rpu_russian.dat`, so mod archive order does not decide which style is displayed. The loader accepts any configured interface width when matching Original and RP FRMs exist; the installed archives provide 640x99 and 800x99 pairs. If Original art is missing but RP art exists, the active value is RP-only. If RP art is missing, the menu explains that `rpu_russian.dat` is needed. If neither source exists at the configured width, the menu hides the RP choice and reports the missing style art. Added FRM decoding and availability fallback tests. Updated the dev snapshot input hook to open the requested dropdown for scripted UI checks.
Blocked: The game was not shown with the exact 3D HUD panel in two safe capture attempts. A scripted attempt to choose RP and save through the options panel did not update `ui.hud_bar_style`; after fixing the script to open the requested dropdown, one further attempt still saved `0`. Selection and restart persistence through the UI therefore remain unverified. Keep PR #416 as draft for owner review.
Next: An interactive check must choose RP in the options menu, save, restart, and show the panel in the 3D HUD. Then recheck persistence and update this task to done/reviewed only after that proof.
Check: Release build completed with `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`; `ctest --test-dir build --output-on-failure` passed 28/28 on the final code. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1` captured all 154 spots; `py -3 tools/regression/check.py --run runs/regression/20261111-233705-389-54392` exited 0, with all spots passing within tolerance (0-2 changed pixels). That pixel regression ran before the later localization and available-source selection fixes, so it does not verify the final binary. Visually inspected classic Original and RP 800px captures, EN and RU options captures, the EN no-RP explanation, the RU RP-only state without `f2_res.dat`, and archive FRM previews. The 1024/4320 decoder cases are synthetic; no matching wide art was present in the installed archives. UI selection/persistence and exact 3D HUD framing were not verified.
