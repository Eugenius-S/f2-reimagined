---
id: 130
title: The 3D options screen finished the way the owner says, ready for the showcase video
milestone: M1
status: blocked
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
Done: The production probe passed with the default windowed config and with a separate test config set to `screen.windowed=0`; a later runtime check showed the latter did not enter exclusive fullscreen, so it is not evidence of full-screen movement. Both runs staged and saved `mouse_sensitivity=2.500000`. The stdout and result JSON are in `runs/130-options-screen-owner-finish/cursor-speed-final/` and `cursor-speed-fullscreen/`. Windowed config line: `PREFERENCES_PROBE CONFIG cursor-speed hash_before=45B1C45A9E1E8C91A9B66EF602515F29FCF77F53504549F876A4681819364C99 hash_after=6AE3DA217D373C3A5676B341CD18C74BB2997CCF08AE56A6544D4A1FBA0852CB zoom=1.000000 enabled=1 combat_difficulty=1 mouse_sensitivity=2.500000`. Fullscreen config line: `PREFERENCES_PROBE CONFIG cursor-speed hash_before=19BE438ED503CAD31D7CBC1C0E343D58953939CA317DAA9E1B894497D21B0BDF hash_after=6AE3DA217D373C3A5676B341CD18C74BB2997CCF08AE56A6544D4A1FBA0852CB zoom=1.000000 enabled=1 combat_difficulty=1 mouse_sensitivity=2.500000`. The full pixel regression passed 154/154; 153 comparisons changed 0 pixels and `newreno-first` changed 2 pixels within its configured tolerance. CTest passed 27/27; scene-export tools passed 145 tests; regression tools passed 12 tests. CI for PR #415 passed, including the Windows game build.
Next: Measure actual cursor displacement at two sensitivity values in both display modes. Probe and config results establish successful setting application in each mode, but visible movement has not been measured; keep `status: blocked` until that evidence exists.
Check: Release build and `ctest --test-dir build --output-on-failure` passed; `tools/dev-snapshot.ps1 -PreferencesProbe cursor-speed ...` passed with both config values, but actual fullscreen was not entered; `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1` passed 154 spots. Fullscreen run used a separate test config with `screen.windowed=0`; the tracked template and game files were not changed.

### 2026-11-11 gpt movement check
Done: Added a runtime displacement assertion through `_mouse_info()`. The default windowed run reported `exclusive_fullscreen=0 relative_mode=0 mouse_lock=0` and passed `cursor_motion mode=absolute sensitivity=1.0 delta=10 sensitivity=2.5 delta=25`. The attempted fullscreen run used an isolated config with `windowed=0`, but runtime still reported `exclusive_fullscreen=0 relative_mode=0 mouse_lock=0`, so this is not a fullscreen movement result. The background runner keeps the app inactive; `mouseDeviceInitMode` only enables the relative path when the app is active and either exclusive fullscreen or mouse lock is set. I did not force activation, capture the owner's focus, or present the absolute-mode result as a fullscreen pass. Logs are in `cursor-speed-motion-windowed-final/` and `cursor-speed-motion-fullscreen-blocked/`.
Next: A separate attempt needs a safe active fullscreen test window to measure the relative path. The task remains blocked and PR #415 stays draft.
Check: Release build and CTest 27/27 passed; `tools/regression.ps1` passed all 154 spots (153 unchanged, 2 pixels within tolerance on `newreno-first`). Windowed motion measured 10 px at sensitivity 1.0 and 25 px at 2.5. Fullscreen movement remains unverified because the background test did not enter exclusive fullscreen.

### 2026-11-11 gpt retry
Done: Rechecked fullscreen through the isolated background snapshot. The process stayed inactive and reported `exclusive_fullscreen=0 relative_mode=0 mouse_lock=0`; setting `screen.windowed=0` did not produce fullscreen. No safe way was found to activate the isolated game without taking foreground focus, so no physical fullscreen input result is claimed. Reviewed the cursor-motion probe: it injects coordinates through `preferencesProbeOverrideMouse` before `_mouse_info()` processes them. This checks sensitivity scaling in the selected code path, not hardware cursor movement. Renamed the probe marker to `injected_cursor_motion` so its PASS cannot be mistaken for physical acceptance. Preserved all untracked diagnostic output.
Next: Keep task blocked and PR #415 draft until an isolated active fullscreen test can be run without taking the owner's focus.
Check: Background fullscreen attempt: `exclusive_fullscreen=0 relative_mode=0 mouse_lock=0`; injected path logs are in `runs/130-options-screen-owner-finish/cursor-speed-motion-fullscreen-final/`. Build attempt: `cmake --build build --config Release --target fallout2-ce` failed in the existing MSVC/SDL headers with missing `sys/types.h`; CTest was not run against this source revision. `git diff --check` passed.
