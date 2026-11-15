---
id: 309
title: Only one launcher worker changes a game folder at a time
milestone: M1
status: todo
owner: any
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: [307]
reviewed: no
---

## Goal

Two launcher processes cannot update, restore or remove the same installation simultaneously; separate game folders remain independent.

## Context

- Standalone cross-process protection: 308's `g_busy` exists only inside one window. Neither `launcher_main.cc` nor `launcher_ui.cc` currently has a per-root mutex/file lock. Two UI windows or CLI workers can race their copies, backup cleanup and removal.
- `ce/src/launcher_main.cc:1475-1501` resolves the game root, removes the old launcher and dispatches uninstall, translation removal and rollback before the normal update path. Acquire ownership before this first mutation, not just around the update copies.
- Follow the single installer chain through 307. Use a small native OS lock with lifetime tied to the worker; no service, global machine-wide serialization or polling framework.

## Acceptance

- While a deliberately paused fixture worker owns a root, a second worker attempting update, RP/translation installation or removal, uninstall, or rollback reports that folder busy and changes no files. Protect interrupted-update recovery and launcher cleanup too.
- Relative/absolute paths, case differences and directory aliases resolving to the same physical installation share ownership. Two genuinely different roots can proceed independently.
- A lock holder that exits or crashes releases ownership automatically; a subsequent worker can enter recovery. A read-only release check does not take the mutation lock.
- Add a focused two-process scratch-folder test for exclusion, alias identity, different-root independence and abandoned ownership. CTest and the existing install/update/rollback fixture pass.

## Out of scope

The renderer/perf lock (290), stopping the player's game, serializing all installations globally, or replacing 307's rollback with a lock.

## Log

- 2026-11-15 gpt: cut from the unguarded worker entry path. Test concurrent scratch workers before changing the filesystem boundary.
