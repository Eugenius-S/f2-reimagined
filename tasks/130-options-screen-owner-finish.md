---
id: 130
title: The 3D options screen finished the way the owner says, ready for the showcase video
milestone: M1
status: doing
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [112]
reviewed: no
---

## Goal

The options screen with the 3D panel looks finished to the owner, so it can
be shot for the showcase video (097, segment 11: every 3D feature switched
off one by one, a slider moved).

## Context

- The owner, 2026-11-01: the screen needs more work before it goes into the
  video, done to the owner's notes. The notes come in chat; the agent asks
  the owner for them when it takes the task and copies them here, with the
  pictures in `runs/inbox/`.
- Builds on 112 (buttons under the panel, straight scrollbar, side frame,
  shadow density 0.8; PR #158) and the layout of 017
  (`runs/017-options-menu-3d-panel-owner-brief.md` on the owner's machine).
  Code: `ce/src/scene/options_panel.cc`.
- Owner note, 2026-11-04 (folded in, the UI batch): a cursor movement speed
  setting: the settings also need the cursor's movement speed. Check first whether the existing `[preferences]
  mouse_sensitivity` in `fallout2.cfg` can carry it: CE ignores it for the
  absolute cursor in a window, so the speed may need its own scaling of the
  cursor's motion there.

## Acceptance

- Every note of the owner's copied into Context holds; before | after of
  the whole options screen at 16:9 and 4:3 in
  `runs/130-options-screen-owner-finish/`.
- A cursor speed slider in the settings visibly changes how fast the
  in-game cursor moves, in windowed and in fullscreen, and is saved in
  `fallout2.cfg`.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The game's own preferences page art, the video itself.

## Log

### 2026-11-05 gpt
Done: Added the cursor speed setting and windowed absolute-cursor scaling; the English and Russian 16:9 and 4:3 panel comparisons are in `runs/130-options-screen-owner-finish/`. Release build and CTest passed earlier (20/20). The initial production probe failed to stage a slider value. A second attempt, adjusted to start at the track midpoint as the existing zoom probe does, failed identically. Both isolated probe logs are in `runs/130-options-screen-owner-finish/cursor-speed-probe/` and `cursor-speed-probe-2/` in the worktree; they are not presented as runtime proof.
Next: Diagnose why the production probe's simulated mouse position does not reach the cursor speed slider, then verify saved config and windowed/fullscreen movement before claiming acceptance.
Check: `tools/dev-snapshot.ps1 -PreferencesProbe cursor-speed`; both attempts reported `PREFERENCES_PROBE FAIL: cursor speed slider did not stage a new value`.

- 2026-11-08 glm: the cursor speed slider landed as PR #292 (model-level tests green; the production probe twice failed to stage a value, so the end-to-end drag acceptance stays open).
- 2026-11-10 claude: back to todo: no open PR and no work since the last log line; the card leaves «В работе».

### 2026-11-11 gpt
Done: Fixed the cursor-speed probe to scroll the row into the viewport before injecting the drag. The probe now guards the slider position against the window bounds. The first diagnostic capture showed row 3 at y=776 in a 540-pixel viewport, so the simulated pointer missed the control. After scrolling was fixed, the value staged from 1.000000 to 2.500000; the injected Done click was then scaled by the new sensitivity and missed, so the probe now commits through Return. The final production-path run passed, verified runtime sensitivity 2.500000, and saved `mouse_sensitivity=2.500000` in the isolated config. Evidence is in `runs/130-options-screen-owner-finish/cursor-speed-diagnostic/`, `cursor-speed-fixed/`, and `cursor-speed-final/`.
Next: Finish the pixel regression, then verify cursor movement in windowed and fullscreen display modes. Do not mark the task done until both are evidenced.
Check: `tools/dev-snapshot.ps1 -PreferencesProbe cursor-speed -CaptureConsole -OutputDirectory runs/130-options-screen-owner-finish/cursor-speed-final -TimeoutSeconds 120` PASS; `ctest --test-dir build --output-on-failure` PASS, 27/27. `tools/regression.ps1` is still capturing its 154 spots; last checked 59 directories, 58 captures, 0 errors. Runtime cursor movement in windowed and fullscreen is not yet verified.

### 2026-11-11 gpt follow-up
Done: The production probe now passes in both the default windowed configuration (`screen.windowed=1`) and an isolated exclusive-fullscreen configuration (`screen.windowed=0`). Both staged and committed cursor speed from 1.000000 to 2.500000, and both isolated configs saved `mouse_sensitivity=2.500000`. The stdout and result JSON are in `runs/130-options-screen-owner-finish/cursor-speed-final/` and `cursor-speed-fullscreen/`. Exact config lines: `PREFERENCES_PROBE CONFIG cursor-speed ... mouse_sensitivity=2.500000` for both runs. The full pixel regression passed 154/154; 153 comparisons changed 0 pixels and `newreno-first` changed 2 pixels within its configured tolerance. CTest passed 27/27; scene-export tools passed 145 tests; regression tools passed 12 tests. CI for PR #415 passed, including the Windows game build.
Next: Measure actual cursor displacement at two sensitivity values in both display modes. Probe and config results establish successful setting application in each mode, but visible movement has not been measured; keep `status: doing` until that evidence exists.
Check: Release build and `ctest --test-dir build --output-on-failure` passed; `tools/dev-snapshot.ps1 -PreferencesProbe cursor-speed ...` passed in both display configurations; `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1` passed 154 spots. Fullscreen run used a separate test config with `screen.windowed=0`; the tracked template and game files were not changed.
