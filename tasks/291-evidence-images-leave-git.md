---
id: 291
title: New evidence images stop growing the repository
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
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
