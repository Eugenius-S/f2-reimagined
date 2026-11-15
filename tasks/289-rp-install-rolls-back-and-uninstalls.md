---
id: 289
title: The Restoration Project install can be undone, by a failure or by the player
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [287]
reviewed: yes
---

## Goal

A Restoration Project install that fails puts the game back as it was, and removing the mod also takes RP out and returns the files it moved to `backup/rpu`.

## Context

- `installRestorationProject` in `ce/src/launcher_main.cc` moves `patch000.dat`, `f2_res.dat` and the loose `data/` files to `backup/rpu`, then copies about 900 MB. A copy cut short leaves the game without its patch and without RP until a later try succeeds.
- `uninstallMod` does not touch RP: `mods/rpu.dat` stays, the original files stay in `backup/rpu`, and the message says the original files were not touched. `launcher_ui.cc` says RP cannot be taken off from the window.
- One restore routine serves both cases. RP's own files are the package's file list (`launcher_rp::targetOf`); keep that list at install time so the removal knows what to delete.

## Acceptance

- A test install from `--rp-zip-url` with a copy failure staged mid-way ends with the game folder equal to its state before (compare file lists and hashes).
- Install RP, then uninstall: the folder equals the clean game it started from, saves untouched; the RP box in the window can be unticked.
- A player who installed RP by hand (no `backup/rpu`) is left alone, with a line saying so.
- ctest passes; rules covered in `ce/tests/launcher_progress/`.

## Out of scope

The Russian translation's install and removal (they work).

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 4 and 5).

### 2026-11-15 gpt

Done: RP installation and removal now use an installer-owned hash manifest in `backup/rpu`, checked atomic file replacement, and one restore routine. Failed copies restore original files; manual RP and changed player files are preserved. The RP checkbox removes managed RP, refreshes vanilla scenes with a checked exporter run, and blocks Play while recovery is pending. Interrupted backup cleanup retains a retryable restored receipt. Saves, added mod-order lines and external hardlinked files are preserved. Both READMEs describe removal; follow-up context stays in tasks 296 and 308.

Check: Release build; CTest 29/29, including 26 scratch RP integration cases; exporter 148 and regression-tool 12 Python tests. Pixel regression 154/154 passed: 150 exact matches, four with 2/2/1/1 changed pixels within unchanged thresholds. Real English/Russian checkbox outcomes are in `runs/289-rp-install-rollback/`.

Next: merge only on final green CI, then complete Rule 1 for fresh merged main before handing over the verified build.
