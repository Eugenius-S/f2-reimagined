---
id: 308
title: The launcher window keeps an installation tied to its game folder
milestone: M1
status: doing
owner: claude
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: [289]
reviewed: no
---

## Goal

While a worker is installing or removing files, late release replies, path edits and closing the window cannot detach it from the UI's operation state or start the game halfway through.

## Context

- Standalone UI race follow-up to the completed installer tasks: 287/289 repair worker filesystem failures, not the window's operation state. Start after 289's RP checkbox changes.
- `ce/src/launcher_ui.cc` disables buttons but not the root text field. `WM_RELEASE_INFO` enables Play/Install before checking `g_busy`. Task 289 adds a busy guard to `launchGame`, so the handler rejects Play during an operation even if a late reply enables the button. Root edits, stale callbacks, busy-close handling and the full acceptance fixture below remain task 308.
- `startOperation:597-621` snapshots a root for the detached worker, but UI disk checks read the editable root. `WM_DESTROY:1128-1136` exits without a busy-close path, leaving the child operation without its window. Treat close handling as part of this same UI operation lifecycle, not a separate framework.

## Acceptance

- With a delayed local release response arriving during a slow fixture install, Play and all conflicting controls stay disabled, and their handlers also reject the operation. The worker's selected root cannot change while busy.
- When idle, changing the root refreshes installed/version/RP/translation state for that root; stale completion/release callbacks cannot apply another root's result or show its success.
- Closing the window while busy uses a clear bounded policy that keeps the active write supervised, such as deferring close until completion. Do not abruptly kill a worker during filesystem replacement and do not leave a silent detached install after the window disappears.
- Exercise delayed release, root change and busy-close sequences against scratch folders. Include a small deterministic state check, CTest, and at most four English/Russian outcome images. Existing install/update/remove/Play flows still work.

## Out of scope

Window redesign, a new UI framework, immediate cancellation of writes, worker rollback (307), or exclusion between separate launcher processes (309).

## Log

- 2026-11-15 gpt: cut from concrete UI state transitions; live race reproduction still needs a controlled local fixture.
- 2026-11-18 claude: claimed for the Win32-free part first. launcher_ui.cc overlaps 307/309 and waits for an explicit handoff of their heads; until then only the state header and its deterministic test.
- 2026-11-18 claude: part 1 of 2 in PR 533, the Win32-free state. **Not integrated:** `launcher_ui.cc` is unchanged and waits for an explicit handoff of the 307/309 heads.
  - **Done.** `ce/src/launcher_ui_state.h`:
    - `controls()`: everything is off while busy, whatever reply arrives. Play needs an installed game and no pending recovery. Install needs a folder and the launcher.
    - `Lifecycle`: `start` refuses a second operation; `changeRoot` refuses while busy and otherwise bumps `rootGeneration`; `releaseMayAct(asked)` is true only when idle and for the same folder; `requestClose` defers while busy and `finish` then reports that the deferred close is due.
  - The test is `ce/tests/launcher_progress/launcher_ui_state_test.cc`, registered as the CTest `launcher_ui_state_tests`. It plays through four sequences: a delayed release reply during a slow install, a folder change, and a close while busy, then idle. Not compiled locally (no compiler outside the heavy slot); CI builds and runs it.
  - **Next, after the handoff,** wire `launcher_ui.cc`:
    - one `refreshControls()` replaces the scattered `EnableWindow` calls, with 307's `updatePending()` going into the `playBlocked` argument;
    - make the folder field read-only while busy, and have `chosenRoot()` return the operation's folder while busy;
    - on `EN_CHANGE` while idle, run `changeRoot()`, refresh the installed, version, RP and translation state, and clear `g_operationCompleted`;
    - `WM_RELEASE_INFO` carries the generation it was asked under and does nothing while busy;
    - `WM_CLOSE` while busy defers, says so in the status and the log, and `WM_OPERATION_DONE` then closes.
  - **Still missing:** the scratch-folder fixture, CTest, and the outcome images, all in the heavy slot.
