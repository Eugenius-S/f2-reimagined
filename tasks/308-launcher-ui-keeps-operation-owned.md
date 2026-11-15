---
id: 308
title: The launcher window keeps an installation tied to its game folder
milestone: M1
status: todo
owner: any
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
