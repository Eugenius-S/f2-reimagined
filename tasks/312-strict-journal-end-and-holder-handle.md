---
id: 312
title: The update journal takes only a plain END count, and the lock test's holder checks its handle
milestone: M1
status: review
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: [292, 307, 309, 311]
reviewed: no
---

## Goal

Two defects confirmed by the independent review of 519/522 are closed, each with a regression test:

- the journal guard no longer reads `END 1junk` as a count of 1;
- the lock test's separate holder process no longer reports READY after a failed open.

## Context

- Stacked on 311 (PR 537, `508c25fd`), which stacks on 309, 307 and 292. Merge only after them.
- `ce/src/launcher_update.h`, `intendedSetCorrupt`: `std::stoull(line.substr(4))` reads the number at the start of the token. It ignores a suffix, leading spaces and a `+` sign, and wraps `-1` around.
- `ce/tests/launcher_progress/launcher_lock_test.py`:
  - the holder sets `restype = c_void_p` and tests `if not h`. A failed `CreateFileW` returns `INVALID_HANDLE_VALUE`, which is a nonzero pointer, so the holder writes READY;
  - its flags `0x400` were meant as DELETE_ON_CLOSE but are not that flag;
  - the parent wrote the lock file only after it started the holder, which opens with OPEN_EXISTING.
- Reproduced before the fix:
  - the old holder, run on a missing file, printed `READY` and exited 0;
  - the first commit, with tests only, puts the new checks red in CI.

## Acceptance

- `intendedSetCorrupt` takes the END count only as the whole token in plain decimal digits that fits `size_t`. Each of `1junk`, `1 `, ` 1`, `+1`, `-1`, `1.0`, `0x1`, an empty count, and the overflowing values blocks recovery. A CRLF journal with a plain count stays good.
- The native `test_end_count_with_a_suffix_is_blocked` checks the real launcher: it blocks recovery, keeps the marker, and changes no file.
- The holder:
  - has `argtypes` and `restype` set;
  - reports FAIL with the Windows error on INVALID_HANDLE_VALUE or NULL, and READY only on a valid handle;
  - opens with no flags (0x80, normal), so a killed holder leaves a stale lock file behind;
  - `test_holder_without_the_lock_never_says_ready` proves the FAIL path.
- The killed-holder test now:
  - writes the lock file before it starts the holder;
  - asserts the contender is refused with ERROR_SHARING_VIOLATION, and closes the contender's handle only if it is a valid one;
  - asserts the stale file still stands after the kill and that the next worker installs anyway.
- CI's `windows-game` job passes. The only production change is the END parser.

## Out of scope

The stream-error distinction after a valid END line that the same review mentions. It was not reproduced and is not a confirmed defect; it needs its own case first.

## Log

- 2026-11-18 claude: cut from `review-519-522-source-delta.md`. Tests first, so CI shows the failures; the fix follows in the same PR. No local build, game or perf.
