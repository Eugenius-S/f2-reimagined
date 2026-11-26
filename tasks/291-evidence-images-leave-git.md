---
id: 291
title: New evidence images stop growing the repository
milestone: M1
status: done
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

A task's before and after pictures no longer add to the git history; the repository keeps the words and the few pictures the READMEs show.

## Context

- `runs/` is 810 MB in a checkout and the pack is 3.1 GiB; CI already skips `runs/` on checkout. `tools/check_runs_size.py` caps one file at 1 MB, not the total.
- Pick the smallest store that works for three agents: a release named `evidence` with one zip per task, or PR attachments. Decide in the Log before building.

## Acceptance

- AGENTS.md says where evidence goes; `check_runs_size.py` fails a PR that adds more than a stated total of images under `runs/`.
- One real task's evidence stored the new way and linked from its Log.
- History is not rewritten; the existing files stay.

## Out of scope

Shrinking the existing history.

## Log

- 2026-11-15 claude: cut from the repo audit of 2026-11-15 (find 7).

- 2026-11-27 gpt: use an evidence release separate from Latest, with one uniquely named ZIP per larger task capture. Keep small inline crops in Git with a 2 MiB image total per PR and the existing 1 MiB per-file cap.

- 2026-11-27 gpt: implemented the 2 MiB changed-image total and retained the 1 MiB per-file check. Exact renames and untouched files do not count. Focused tests cover the limits and existing evidence. Next: Main publishes and verifies a real task ZIP in the separate evidence release, links it here or in that task Log, and completes review. Status remains doing.

- 2026-11-27 gpt: CI checks the existing 1 MiB per-file limit and the 2 MiB
  aggregate of changed images. The checks count changes to Git file type
  when the result is an evidence file. Unchanged evidence and exact moves
  keep their existing allowance. Four focused tests passed; independent
  Terra High reviewed the corrected checker at `0d0ba04c`.
- The [322 cursor captures](https://github.com/Eugenius-S/f2-reimagined/releases/download/evidence/322-cursor-qa-c2f5a723.zip)
  are linked in the 322 Log in PR #607. The ZIP holds six frames and their capture
  timing notes. The downloaded SHA-256 is
  `c1d78168f66456acac24a4a695495b6892c8cbff9d81cb22843231da357d51dd`.
  The default player release stays Latest at `a414e39d`. No existing
  evidence or Git history was moved.
