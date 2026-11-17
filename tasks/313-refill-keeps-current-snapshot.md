---
id: 313
title: A same-version refill rolls back to a fresh snapshot of the current build, never to an older backup
milestone: M1
status: review
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: [292, 307, 309, 311, 312]
reviewed: no
---

## Goal

A same-version wide-art refill that fails restores the bytes that stood right before it. It never rolls back to an older backup that happens to be valid.

## Context

- Stacked on 312 (PR 538, `d9db1175`), which stacks on 311, 309, 307 and 292. Merge only after them.
- Found by the independent review `review-519-522-source-context.md` (P1).
- `ce/src/launcher_main.cc`, the refill branch of the update. When no backup named after the installed version stands, it calls `backupInstalledBuild(root, "manual", ..., true)`. That call returns an existing valid `manual` backup as it is, without copying the current state.
- The real path to such a backup:
  1. a stamp-less install updates to v1 and keeps `manual` (the old stamp-less bytes);
  2. a later v1 refill reuses that `manual` as its recovery point;
  3. when the refill fails, the restore, or the next `--rollback`, puts the old bytes back over the current v1.
- Reproduced before the fix: the first commit, with the test only, goes red in CI.

## Acceptance

- `test_failed_refill_never_restores_an_older_manual_backup` runs the real path on the real launcher:
  - a stamp-less install updates and keeps `manual`;
  - a refill fails on a held `version.txt`;
  - `--rollback` brings back every current file byte for byte, outside the backups folder;
  - the older `manual` backup stays byte for byte as it was.
- A refill always snapshots the current state into its own folder, `refill-snapshot`. It rebuilds that folder each time and never reuses an older one. It never touches `manual` or a version-named backup.
- If that snapshot cannot be made, the refill refuses before its first write, as before.
- The two older refill tests now name `refill-snapshot` instead of `manual`. Their checks stay the same.
- CI's `windows-game` job passes.

## Log

- 2026-11-18 claude: cut from the P1 in `review-519-522-source-context.md`. Test first, so CI shows the failure; the fix follows in the same PR. No local build, game or perf.
