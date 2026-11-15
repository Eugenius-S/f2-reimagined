---
id: 307
title: A failed build update restores the previous complete build
milestone: M1
status: todo
owner: any
suggested: gpt
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: [292]
reviewed: no
---

## Goal

A failed update leaves the installed build usable and preserves a known-good backup across retries, instead of mixing old and new files.

## Context

- Standalone follow-up to 287: that task explicitly excludes rollback of files already copied. 289 covers RP files, not the build update. Keep one installer worker through 287, 289, 292, then this task.
- `ce/src/launcher_main.cc:739-762` backs up build files into a version-named folder with overwrite enabled. The update copies files one by one at `1582-1602`. After 287, a late failure keeps the old version but may leave newer files: a retry can overwrite the old backup with this mixed state.
- `rollbackInstalledBuild` at `808-859` skips a backup whose version equals the installed version (`831`), so the backup cannot repair that failed same-version state. The backup list also omits asset trees that the update overwrites.
- Reuse the existing backup/staging machinery with the smallest complete restore path; preserve the prior snapshot until commit. Coordinate with 288's change to the package's own version stamp, which touches the same update commit point.

## Acceptance

- A local fixture update failing after the first copy and after a later copy restores the prior build's file list and hashes, including any release-owned files replaced or introduced before failure; its version stays old. A successful retry commits the new build once.
- Repeat a failed update: the original good backup is not overwritten by partial state. Explicit rollback can repair an interrupted update even when its saved version matches the installed stamp.
- If the worker exits between replacement steps, the next launch detects and recovers the interrupted update before it can launch or mutate the half-installed build. A bounded interruption test demonstrates this path.
- If restoration itself is blocked, report the affected file, keep the recovery material and do not report success. After the obstruction is removed, recovery completes. Saves, unrelated mods and player settings stay byte-identical.
- CTest and the local fixture install/update/rollback checks pass. Build on 287's error handling and 289's restore conventions without duplicating their scopes.

## Out of scope

RP rollback (289), download retries/refactoring (292), release identity (288), whole-directory snapshots of player data, or a general transaction framework.

## Log

- 2026-11-15 gpt: code review found the explicit rollback gap after 287; file-failure and interruption scenarios must first be reproduced in a scratch fixture.
