---
id: 311
title: Native launcher tests for the recovery and lock cases 307 and 309 left uncovered
milestone: M1
status: review
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: [292, 307, 309]
reviewed: no
---

## Goal

The real launcher exe proves the recovery and root-lock cases that 307 (PR 519) and 309 (PR 522) cover only in unit tests or not at all. The tests use the existing CLI flags only.

## Context

- Stacked on 309's head `215cf0da`, so the branch carries 292, 307 and 309. Merge it only after PRs 518, 519 and 522.
- No production code changes. The tests reuse the fixtures already in `ce/tests/launcher_progress/`: `LauncherFixture` (a local HTTP release), `RecoveryFixture` (a seeded marker, journal and backup), `read_lock`. CTest already runs these three files, so CMake is unchanged.
- Failures are injected without any new flag:
  - a plain file where `f2reimagined-backups` belongs makes the manual snapshot impossible;
  - a held `version.txt` stops the stamp write and the in-place restore;
  - a local HTTP handler holds the first release request, so a real launcher stays parked after it took the root lock and before any write.

## Acceptance

- `launcher_install_test.py`:
  - `test_refill_without_a_recovery_point_changes_nothing`: a same-version refill with no backup of the build and no way to make one refuses, and the game folder stays byte for byte.
  - `test_failed_refill_rolls_back_from_its_manual_snapshot`: the refill takes its `manual` snapshot and fails at the stamp. The in-place restore is blocked, and the marker names `manual`. Then `--rollback` restores from `manual`: the build files are back, the introduced art is gone, the marker is cleared, and the save, `fallout2.cfg`, `master.dat` and `critter.dat` are untouched.
- `launcher_recovery_test.py`:
  - `test_missing_journal_is_blocked_not_guessed`: a marker without its journal blocks recovery and changes no file.
  - `test_player_files_survive_the_restore`: a restore removes only the journal's introduced file. A save, the player's config and an unlisted file inside `user-assets` stay byte for byte.
- `launcher_lock_test.py`:
  - `test_second_launcher_refuses_a_live_one`: two real launcher processes on one root. The second refuses before any network request or write, and the first then finishes its install.
  - `test_killed_launcher_releases_the_lock`: a real launcher is killed while it holds the root, and the next worker installs.
- CI's `windows-game` job runs the launcher CTest tests, and they pass. Only light checks run locally (py_compile), because the heavy slot belongs to the coordinator.

## Out of scope

- A launcher worker killed in the middle of its copies. The current CLI cannot stop it at a chosen point deterministically: the copies take milliseconds after a download the test cannot pause inside the install. The minimal seam would be a test-only environment variable, such as `F2R_TEST_EXIT_AFTER=<relative path>`, that the copy helpers (`copyIfPresent`, `copyTreeIfPresent`, `copyLauncher`) check after each path lands, and then call `TerminateProcess(GetCurrentProcess(), 3)`. The test would then run a plain worker start and assert the restore from the marker's backup. This is a production change, so it needs coordination first.
- A real disk-full or access-denied failure of the backup write; the folder-shaped obstruction stands in for it.

## Log

- 2026-11-18 claude: cut from the coverage table in the 519/522 acceptance inputs. Six native scenarios were added on top of `215cf0da` in a draft PR, test code only. Only py_compile was run, with no launcher run and no build. The crash-mid-copy case stays a documented gap with the seam above.
