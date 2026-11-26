---
id: 315
title: Fallout 2 starts at 3440 x 1440 after a clean Windows install
milestone: M1
status: blocked
owner: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

On a clean Windows install, Play in the launcher starts the game and reaches a usable menu with the saved 3440 x 1440 mode.

## Context

- The owner reports that after a clean Windows install, choosing 3440 x 1440 in the launcher and launching the game causes it to exit or crash before it is playable. Neither the exit type nor the cause is established.
- This is a startup blocker in the installer/launch batch. Track it separately from 314, which covers launcher self-update, uninstall cleanup and UI presentation. It does not wait for 314.
- The current launcher has no display selector. The game reads its saved resolution at startup.

## Acceptance

- Reproduce the reported sequence with a clean disposable Windows installation, recording the tested build, chosen mode and startup outcome.
- Starting with 3440 x 1440 selected reaches a usable main menu with that supported mode applied, including on the next launch.
- Record an automated regression where practical and keep a standard-resolution startup check. Do not use or change the owner's live installation.

## Out of scope

Launcher self-update, full uninstall and visual cleanup from 314.

## Log

- 2026-11-20 gpt: recorded the owner's report as an urgent tier 0 startup blocker. No diagnosis or implementation started.
- 2026-11-21 gpt: fixed wide viewport object-offset generation by allowing abstract samples outside the map; existing callers still validate tile indices before grid access. Also removed the hires menu panel blit while preserving its label layout bounds, independent button art, hitboxes, and backdrop. Windows Release build and CTest (29/29) pass; scene-export and regression unit suites pass (148 and 12 tests). `tools/dev-snapshot.ps1` reaches the Russian menu at 3440x1440 1x on two consecutive saved-config launches, loads KLADWTWN.MAP at that resolution, and reaches the menu at 1920x1080 1x. Wide and plain menu captures are in `runs/315-menu-wide-3440`, `runs/315-menu-plain-3440`, `runs/315-menu-wide-1920`, and `runs/315-menu-plain-1920`. Full map regression is running.
- 2026-11-26 gpt: resumed acceptance on current main after #547. Verify the released launcher on a clean Windows copy, its saved-resolution restart and a standard-resolution control.

- 2026-11-26 gpt, clean-release acceptance (partial): on the clean worktree at `7beb6a17ca295d91a4ee35004f9129ba8b247a2d` (base `5e8738eab`), installed the official rolling Latest with `Install-F2R.exe --game-root <isolated-copy> --maps kladwtwn --no-start`. The isolated copy contained copied `master.dat` SHA256 `9b096d3035edafd4077deeb8ee7877a803b9db98497aa4596624b5c058a84711` and `critter.dat` SHA256 `da83d615967c0bac41fbc50e6b86b3088401ff69ed9d8b2e5be1d6ac0c125bcd`; no owner saves or installation were used. Installer reported fresh install and release `a414e39d7519d3544a769f4d2235da936192df93`. Official installer SHA256 `058ba6346dd087ab264b62eab63c76dc3f822cd870c7ab62c6ac9dc1ef948f00` and release zip SHA256 `6209c3385a10ba8fbcdefc76a91adb955b73eec511f9400572d04488ed9c1d81` matched GitHub's release asset digests. Only Klamath was exported, not the full default installation.
- The released game executable SHA256 `fd2117abd2bb87b44df1afdc9ba823b3d1fe3a64c71277748de1977be163010b` was copied by `tools/dev-snapshot.ps1` to an isolated test directory. A manually prepared copy of the installed default config selected `resolution_x=3440`, `resolution_y=1440`, `scale_factor=1.0`, `windowed=1`; `[3d] enabled=1` and `zoom=1.0` were added for the harness. Two consecutive hidden, off-screen `-ConfiguredDisplay -ReuseTestConfig -MainMenu wide` runs produced distinct menu frames and JSON (`dee16ed64add4c93970752363b241734`, `98ab98645c9f434b936ddc221bcf4ba4`) with physical, logical and drawable sizes 3440x1440 and scale 1.0. The first run persisted those screen values before the second. A 1920x1080 windowed 1.0 control (`d1c0c533475b4fe7b72f3741f75b0d3e`) produced a matching menu frame and dimensions. Reduced copies of the second wide and control frames are in `runs/315-clean-launch-acceptance/`.
- The snapshot script reported `No snapshot within 90 s` on the second and control runs after about five seconds, but each unique run folder contained its own completed PNG and JSON with the correct run ID and executable hash. The frames were inspected; the report is a harness detection failure, not evidence of a game crash. The game was run directly with dev menu capture, which exits automatically; menu keyboard and mouse operation were not tested. The actual launcher start path and a real visible supported desktop mode remained unverified at this point: config editing plus off-screen rendering is not proof of those steps. The current launcher has no resolution selector; do not treat selecting a resolution there as an acceptance step. Do not mark done until that path is verified.

- 2026-11-26 gpt, private desktop follow-up: created a separate Win32 desktop without switching to it, used direct `CreateProcessW` with `STARTUPINFO.lpDesktop`, and verified the packaged launcher window existed on that desktop before sending its Play control (1014). With auto-updates disabled and the isolated game root and settings, the real Play handler reported `The game is running.` and launched the exact installed `fallout2-ce.exe` above without dev arguments. Two 3440x1440/1x windowed launches used the same retained config; both produced the SDL game window on that desktop and logged `MAINMENU` after startup. A 1920x1080/1x control also produced the game window and logged `MAINMENU`. Game processes were stopped after inspection; this does not demonstrate graceful game exit or game-saved settings. `ImageGrab` failed for the game window on the inactive desktop, so those ordinary Play runs do not prove the rendered menu, actual drawable size or usable keyboard/mouse input. Background dev-capture results above remain separate evidence. The initial attempt used Python subprocess, which silently dropped the custom desktop field; its launcher could have appeared on the ordinary desktop with `--no-activate`, was stopped, and did not launch the game. The corrected runs used direct Win32 and enumerated the private desktop. Next: verify real menu and input on an isolated interactive desktop. Leave the task open.

- 2026-11-26 gpt acceptance: blocked on a safe interactive session for the ordinary Play run. `MAINMENU` above is the background-art load diagnostic, before the menu input loop. The separate hidden captures show the menu and drawable at both sizes. Next, save 3440 x 1440 in the game settings on a disposable copy, exit normally, then start from Play and use the menu. Compare against 1920 x 1080.
