---
id: 301
title: Snapshot output distinguishes the current run from older captures
milestone: M1
status: doing
owner: claude
suggested: glm
priority: normal
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Reusing an output folder never makes older devshots look like results of the current attempt.

## Context

`tools/dev-snapshot.ps1:1339-1348` creates an existing `OutputDirectory` and copies captures into it without retiring the previous capture set. Consumers such as `tour.py` and `regression/check.py` scan capture filenames. This is a bounded follow-up to the [snapshot batch](183-owner-feedback-shot.md) and [tour evidence](225-fly-tour-evidence-flight-view-changes.md), and does not change the separate 268-270 capture fixtures.

## Acceptance

- Track and replace only the script's own previous capture filenames and their paired PNGs, preferably with a small scoped filename list. Preserve unrelated user files, reports and other artifacts.
- On a failed attempt, prior captures cannot be mistaken for the current successful result. Use a clear failed/incomplete state that existing callers can recognize, with nonzero exit; do not silently return old filenames.
- A small harness self-test simulates two successful runs with different capture counts and a failed third run in one folder. The current set is exact, unrelated files survive, and failure is observable.
- Existing snapshot callers and tour/regression tool tests pass. Record what filenames the script owns and how failure is exposed.

## Out of scope

Deleting entire user output folders, a general run-receipt framework, changing engine screenshot format or fixing dialogue/white-ground causes.

## Log

- 2026-11-15 gpt: cut from the tooling and verification follow-up to the existing batches; source observations above are scoped gaps, not unverified game bug claims.
- 2026-11-18 claude: claimed. Own captures listed with hashes in a manifest in the output folder; a failed run exits nonzero and leaves state "incomplete". Code and fake tests only, no game runs.
- 2026-11-18 claude: done in PR 529, code and fake tests only.
  - **Owned files.** `tools/snapshot-output.ps1` keeps `snapshot-output.json` in the `-OutputDirectory`. It lists the files this run copied in, each with its SHA-256: the devshot JSON sidecars, their images, and the `-PreferencesProbe` shots. Logs, `process.stdout.log`, the probe and shadow dumps and the watchdog files are not listed, so they are never deleted.
  - **Reset.** The next run first deletes only the listed files whose hash still matches. A file edited since the last run stays. So do files that are not listed, names with a path, and the folder itself. A damaged manifest stops the run before anything is deleted.
  - **Failure.** Every run writes `state: "incomplete"` as it starts and after each copy, and `"complete"` only in `Complete-RunFolder`, once every check has passed. A failed run exits nonzero, as before, and lists only its own files. Callers keep reading the exit code. A tool that reads the folder later can check the state.
  - **Checks.** `tools/test_snapshot_output.ps1` (39 checks, now in the Windows CI job) runs fake runs in separate processes in one folder:
    - two successful runs, with 3 captures and then 1;
    - a third run that fails after one capture;
    - a run that fails before any capture;
    - unrelated files that must survive, an edited capture that must survive, an escaping name and a damaged manifest;
    - a check that `dev-snapshot.ps1` makes these calls in the right places.
  - **Mutations.** Of 9 mutants of the helper, 7 were killed. The 2 survivors were dead lines, now deleted.
  - **Caller tests.** tour (6) and regression (12) pass, and `dev-snapshot.ps1` parses. Callers read only `devshot-*` names or `*.png`, so the manifest does not disturb them. tour, check.py, gallery_pairs and shapes/eval all read the folder this way.
  - **Not verified:** a real dev-snapshot run. No game was allowed (the slot was taken).
  - **Limits.** Captures written before the manifest existed cannot be proved to be the script's own, so they stay. The first new run in an old folder can still sit next to them. `rp-check.ps1` belongs to GLM (302) and was not touched; it already checks the exit code. Any other compatibility question goes through Main.
