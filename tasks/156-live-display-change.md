---
id: 156
title: Resolution, window mode and scale change on the fly, without a restart
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: [154]
reviewed: yes
---

## Goal

The display rows in the 3D settings (154) take effect when the panel
closes, without restarting the game: window mode, resolution and picture
scale. The "restart needed" notes go away for what now applies live.

## Context

- Folded in (the owner, 2026-11-04, `runs/inbox/2026-11-04-sharp-text-live.png`,
  the panel's Lighting section in the original pixel font): the sharp-text
  switch (025) should apply on the fly like the other settings. Turning it on
  or off changes all text right away (the settings panel itself, dialogue,
  the message log) with no restart and no stall; today it needs a restart.
- Folded in (the owner, 2026-11-04, a screenshot over Klamath): in
  fullscreen, NVIDIA's "Press Ctrl+Alt+Z to open the overlay" toast keeps
  popping up during play, as if the game were starting again. The NVIDIA
  app shows it when a process creates a new D3D or GL device or swapchain in
  fullscreen, so something recreates the presentation device during play.
  Suspects: `svga.cc` `handleWindowSizeChanged()` destroys and recreates the
  SDL renderer whenever the scene renderer is not active (combat, dialogue,
  menus, movies); a switch between the GL scene path and the SDL renderer
  path (if that is the cause, it ends with 173); an exclusive-fullscreen
  mode switch. Same renderer and surface lifetime as this task.
- The owner, 2026-11-02: wants resolution and scale to change on the fly
  and without a stall.
- 154 saves `[screen] resolution_x`, `resolution_y`, `windowed` and the
  scale factor and asks for a restart. Window mode is an SDL window flag
  change. Resolution and scale change the logical framebuffer size
  (`ce/src/svga.cc`), so the SDL texture, the 3D render targets and the 2D
  game windows (interface bar, map view, open dialogs) laid out at start
  for that size must be rebuilt or re-laid out. Find what CE sizes at start
  and choose: rebuild live everywhere, or live from the options screen with
  the map window re-created on return. Write the choice in the Log first.
- A mode that fails falls back to the previous one, as in 154.

## Acceptance

- Scripted input (154's dev input) switches 1920x1080 1.4x window to
  2560x1440 2.0x and to borderless and back in one run, with frames after
  each switch in `runs/156-live-display-change/`: the interface fits, the
  cursor lands where it points, the 3D view fills the window.
- Every 3D settings row (shadows, glow, clouds, lights, scale, resolution)
  applies without a stall: measured with 127's frame-time log, no frame
  over 100 ms on the change. A change never re-exports or re-reads the scene
  cache; it rebuilds only the render targets or shader state it touches.
- The switch takes under a second; the original view regression stays
  pixel for pixel; ctest passes.

- One presentation device for the whole run: every renderer and context
  create and destroy is logged with its reason; a fullscreen session
  through a map change, a dialogue, combat and the options screen logs no
  recreate after launch, and the overlay toast shows at most once.

- Sharp text toggled with the panel open: the panel, a dialogue and the
  message log before and after, no frame over 33 ms around the toggle.

## Out of scope

Speed of the game in general (157).

## Log

- 2026-11-02 Cut from the owner's request; 154 asked for a restart.

- 2026-11-08 glm: PR #227 closed by task 221: #329 already applies the window mode live; the rest (live resolution and scale, task 209) is a 2356-line transaction blocked in its own runtime validation (two consecutive failures at refresh stages) and cannot land unvalidated. The branch's implementation-path and blocker notes are preserved in this Log.

### 2026-11-08 glm: the rescue run preserved the branch (PR #227) Log below

### 2026-11-08 glm: the rescue run preserved the closed branch (PR #227) Log below

### 2026-11-04 gpt: release after runtime blocker
Done: rebased the branch on current `origin/main` and retained the draft PR. The current-source scripted transactions from task 196 saved the display settings in about 17 ms, but they used the static wall-and-hero preview and did not capture the requested live resolution, window-mode, scale, pointer or regression behavior. Attempts 7 and 8 remain two consecutive runtime failures at different refresh stages. No further same-path runtime retry is justified without a diagnosed fix.
Next: return to this task when the intermittent display-buffer/window refresh stop has a concrete fix; do not treat this task as complete.
Check: PR #227 CI run 41817508524 on `a0dc37f2` (Windows build pending at handoff); prior local Release build and CTest passed, runtime acceptance is still missing.

## Goal

The display rows in the 3D settings (154) take effect when the panel
closes, without restarting the game: window mode, resolution and picture
scale. The "restart needed" notes go away for what now applies live.

## Context

- Folded in (the owner, 2026-11-04, `runs/inbox/2026-11-04-sharp-text-live.png`,
  the panel's Lighting section in the original pixel font): the sharp-text
  switch (025) should apply on the fly like the other settings. Turning it on
  or off changes all text right away (the settings panel itself, dialogue,
  the message log) with no restart and no stall; today it needs a restart.
- Folded in (the owner, 2026-11-04, a screenshot over Klamath): in
  fullscreen, NVIDIA's "Press Ctrl+Alt+Z to open the overlay" toast keeps
  popping up during play, as if the game were starting again. The NVIDIA
  app shows it when a process creates a new D3D or GL device or swapchain in
  fullscreen, so something recreates the presentation device during play.
  Suspects: `svga.cc` `handleWindowSizeChanged()` destroys and recreates the
  SDL renderer whenever the scene renderer is not active (combat, dialogue,
  menus, movies); a switch between the GL scene path and the SDL renderer
  path (if that is the cause, it ends with 173); an exclusive-fullscreen
  mode switch. Same renderer and surface lifetime as this task.
- The owner, 2026-11-02: wants resolution and scale to change on the fly
  and without a stall.
- 154 saves `[screen] resolution_x`, `resolution_y`, `windowed` and the
  scale factor and asks for a restart. Window mode is an SDL window flag
  change. Resolution and scale change the logical framebuffer size
  (`ce/src/svga.cc`), so the SDL texture, the 3D render targets and the 2D
  game windows (interface bar, map view, open dialogs) laid out at start
  for that size must be rebuilt or re-laid out. Find what CE sizes at start
  and choose: rebuild live everywhere, or live from the options screen with
  the map window re-created on return. Write the choice in the Log first.
- A mode that fails falls back to the previous one, as in 154.

## Acceptance

- Scripted input (154's dev input) switches 1920x1080 1.4x window to
  2560x1440 2.0x and to borderless and back in one run, with frames after
  each switch in `runs/156-live-display-change/`: the interface fits, the
  cursor lands where it points, the 3D view fills the window.
- Every 3D settings row (shadows, glow, clouds, lights, scale, resolution)
  applies without a stall: measured with 127's frame-time log, no frame
  over 100 ms on the change. A change never re-exports or re-reads the scene
  cache; it rebuilds only the render targets or shader state it touches.
- The switch takes under a second; the original view regression stays
  pixel for pixel; ctest passes.

- One presentation device for the whole run: every renderer and context
  create and destroy is logged with its reason; a fullscreen session
  through a map change, a dialogue, combat and the options screen logs no
  recreate after launch, and the overlay toast shows at most once.

- Sharp text toggled with the panel open: the panel, a dialogue and the
  message log before and after, no frame over 33 ms around the toggle.

## Out of scope

Speed of the game in general (157).

## Log

- 2026-11-02 Cut from the owner's request; 154 asked for a restart.

### 2026-11-02 gpt: implementation path

Apply display changes only after the preferences window is destroyed, then rebuild the map and interface windows for the new logical dimensions without unloading the map or objects. `svga.cc` owns the logical SDL surfaces and presentation, the window manager owns a screen-sized backing buffer and root rectangle, `map.cc` owns the iso window and tile/object buffer pointers, and `interfaceInit` lays out the bottom bar. The GL presenter holds logical dimensions while render targets adapt to drawable size; mouse input caches the window-to-logical mapping. Open dialogs and menus must not be left with old-sized buffers: defer the transaction until the preferences window and any enclosing modal screen are closed. For window mode use SDL flags on the existing window. Preallocate the new surfaces and window buffers, validate the mode, then swap and rebind these pieces; if any stage fails restore the previous window mode, dimensions, surfaces, settings and pointers. Measure the complete operation and verify actual rendered frames and cursor mapping before declaring success.

### 2026-11-02 gpt: paused, build environment

PR #227 remains draft. Live display changes are not connected: the current
settings still require a restart. Reversible SDL window, surface, root buffer,
map window and tile/map/text view preparations have been committed. The dev
input runner can reopen preferences in one process. The last completed test
run before the object-table work passed 21/21 tests; the reopen diagnostic
frames only prove pending settings, not a live switch. The acceptance run,
switch timings, pixel regression on the completed change and CI remain open.

Stopped after repeated MSVC build environment failures. A plain
`cmake --build build --config Release --parallel 4` failed with
`fatal error C1083: stdio.h: No such file or directory`. Running the same
build after `VsDevCmd.bat -arch=x64` failed with `vcruntime.h: No such file
or directory`; VsDevCmd supplied the Windows SDK include paths but omitted
the MSVC include path. A further attempt to supply the MSVC path through
`cmd` lost the SDK include paths and again failed on `stdio.h` and `assert.h`.
No further build attempts were made. The unfinished, untested object-table
work was saved locally as `build/task156-object-view-unverified.patch` and
is not part of the PR. Build output is in `build/task156-object-build.log`
and `build/task156-object-build2.log`.

For continuation, the object renderer needs new viewport-dependent scratch
tables without clearing the map or player. Reposition the existing interface
bar without recreating it, preserving its buttons and state; prepare its side
panels and indicator placement as part of the same rollback-safe transaction.
Complete the modal-close integration and validate the requested one-process
runtime sequence before removing restart labels or changing this task to done.

### 2026-11-03 gpt: paused after build environment failures

Done: restored the saved object-view swap work from
`build/task156-object-view-unverified.patch`. It adds the reversible object
renderer table exchange needed by the display transaction.
Next: resume only after the Windows build environment is repaired; finish
interface-bar repositioning and modal-close integration, then run the full
single-process display-change acceptance sequence. PR #227 remains draft.
Check: `git apply --check` passed. Three consecutive local builds failed before
compiling this change: MSVC headers were initially missing from `INCLUDE`; after
adding them, UCRT `assert.h` and `stdio.h` could not be resolved. Existing CI is
green, but this implementation has no successful build or live runtime proof.

### 2026-11-03 gpt: runtime transaction blocked

Integrated a live display transaction after the preferences window and its
enclosing options screen close. It prepares and exchanges SDL surfaces, the
root and iso-window buffers, tile/object/text views, then moves the existing
interface bar while recreating only its side-art windows. Display settings now
use `OnClose`; failed application restores the previous values. The clean local
MSVC build passed and CTest passed 21/21.

Two consecutive background acceptance attempts used the isolated 1920x1080,
1.4x windowed config and the task 154 scripted input. Both saved the requested
2560x1440, 2.0x values but stopped before the first frame after saving. The
second diagnostic run reached the SDL window change, object viewport-table
preparation and indexed/RGBA surface exchange; it produced no marker after the
surface exchange and timed out before the fifth snapshot. No post-change visual,
cursor, timing or regression proof exists. The separate fourth probe had a
malformed duplicate screen section and is excluded from the two-attempt count.
Logs: `runs/156-live-display-change/live-attempt2/` and
`runs/156-live-display-change/live-attempt3/`.

Validation is blocked; task status remains `doing` per the task schema,
`reviewed: no`. PR #227 remains draft. The implementation
is pushed for inspection, but this task is not ready for review until the
display-buffer exchange stop is diagnosed and the same-process acceptance run
captures every requested mode.

### 2026-11-03 gpt: merge origin/main

Merged `origin/main` at `d8cff6b0` into `task/156-live-display-change`. Resolved the scripted-input conflicts so scene-only auto-start and options-panel preferences input both remain available; retained the task-156 display transaction request, `waitms` and key actions, and the old `DevInputSnapshots` command name as an alias. The result metadata still records the window mode.

Sanitized the absolute worktree cache path in the attempt 2 and 3 logs. Release build passed with MSVC 14.40 and Windows SDK 10.0.22621.0; CTest passed 23/23. The scripted-input PowerShell file parses, and `git diff --check` passes.

Next: continue diagnosing the existing display-change runtime timeout and capture the requested live frames. The task remains `doing`, `reviewed: no`, and PR #227 remains draft until runtime acceptance is proven.

### 2026-11-04 gpt: fresh-main runtime retry

Merged `origin/main` at `3b288fd6` and pushed merge SHA `53b201c6`.

A fresh Release rebuild with MSVC 14.40 and Windows SDK 10.0.22621.0 passed; CTest passed 23/23. A new isolated one-process scripted run (#6) completed the display transaction from windowed 1920x1080 at 1.4x to 2560x1440 at 2.0x. Its script reported PASS with 5/5 captures. The first and last sidecars record the requested dimensions and drawable sizes; the final frame is `runs/156-live-display-change/live-attempt6/frame-after-2560x1440.jpg`. The renderer trace reached `settings-saved`.

This run used the static wall-and-hero 3D preview (`gameState=0`), so it does not establish behavior in a loaded playable game. It also did not change window mode or return to the initial mode and scale. The end-to-end switch time, pointer alignment and origin-main pixel regression remain unverified. Stage timing was added to the devtools-only trace; the later failed run measured only 12 ms through interface repositioning, not a complete transaction.

Two consecutive new attempts then failed. Multi-step run #7 saved the requested config but stopped after interface repositioning, before the tile-center/window refresh and the fifth frame (4/11 captures). Diagnostic run #8 stopped after screen-buffer exchange and before the iso-window exchange (4/5 captures). The trace stops at different stages, so the failure is intermittent and not yet isolated. Logs and before/after sidecars are retained in `runs/156-live-display-change/live-attempt6/`, `live-attempt7/` and `live-attempt8/`.

Per the two-consecutive-failure limit, stop runtime retries here. PR #227 remains draft and the task remains `doing`; do not claim acceptance. Continue the ordered queue with task 127.

### 2026-11-10 glm: state of the task and the one-device audit

What already applies live on main and no longer needs this task: window
mode (#329, applyWindowModeLive at the panel close), sharp text (its
descriptor is OnClose), and every 3D row the acceptance names except two -
shadows, glow, clouds, lights, fps limit, map clip all apply on close;
the settings_3d suite gates them. The "Requires restart" note remains on
exactly two descriptors: scale_factor and resolution_mode (the only
SettingApplyPolicy::Restart registrations left in settings.cc).

One presentation device: audited the lifetime on main. The GL context is
created once at startup (_GNW95_init_mode_ex -> scene_renderer::init) and
never leaves. handleWindowSizeChanged on the healthy path only refreshes
the mouse mapping; destroyRenderer (with scene_renderer::shutdown) runs
only when the GL init failed and the SDL fallback is in charge, and
movieHandleRendererReset only frees the movie overlay texture. So on a
normal install nothing recreates the presentation device during play, and
the NVIDIA overlay toast suspect (the 2026-11-04 note) does not exist on
the GL path. No code change made.

The remaining slice is the live resolution+scale transaction. The
2356-line table-exchange branch (PR #227, closed) failed twice at
different stages of the same transaction; the intermittent stop was never
diagnosed. My recommendation for the return to this task: do not resume
that branch. Re-enter at the preferences close with a rebuild that reuses
the engine's own init paths instead of exchanging buffers live -
directDrawInit already supports re-init with palette preservation, the GL
side needs its logical size re-set plus a new gSdlTextureSurface, and the
GNW window layer should be torn down and re-created through
interfaceInit/map window init rather than repositioned in place (the
acceptance explicitly allows "live from the options screen with the map
window re-created on return"). Owner question recorded here rather than
asked live: whether the under-a-second budget tolerates a brief black
frame on the switch.

- 2026-11-12 claude: read the device lifetime on main `559a3710`, no run. The first suspect in Context no longer holds: `scene_renderer::isActive()` is true from `scene_renderer::init` at window creation to exit, so `handleWindowSizeChanged()` never destroys the renderer on the GL path, and there is one `SDL_GL_CreateContext` call in the tree (`scene_renderer/context.cc`). The SDL renderer is created only when GL init fails. What is left for the overlay toast is the exclusive fullscreen mode itself (`SDL_WINDOW_FULLSCREEN`: focus loss and return re-enter the mode) and `applyWindowModeLive()`. Next for whoever takes it: log `SDL_WINDOWEVENT_FOCUS_*` and display-mode changes in a fullscreen session and compare with borderless, where the toast should not repeat.
