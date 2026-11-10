---
id: 254
title: The tools folder holds only tools in use, and the long-dash rule is checked
milestone: M1
status: todo
owner: any
suggested: glm
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A new agent finds the working tools at a glance, and the rule against the
long dash stops depending on memory.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 14).
  Stands alone: 198 cleans the owner's projects folder, not the repository.
- `tools/` holds about 40 one-off scripts of finished tasks (`shots119.ps1`,
  `grid166.ps1`, `probe167.ps1`, `pairs120c.py`, `classic169b.ps1`,
  `walk169.ps1`, `hours166.ps1`, `reg119.ps1`, `effect_white_review.py`,
  `glow_ladder_sheets.py` and their kin). The ten checked have no reference
  outside `runs/` and `tasks/`.
- The long dash (U+2014) is in 24 tracked text files, the title of task 246
  among them. `tools/check_public_content.py` does not look for it.
- Task 225's `suggested` field holds a phrase.

## Acceptance

- Every script in `tools/` (top level) either has a reference outside
  `runs/` and `tasks/` (AGENTS.md, docs, another script, the workflow) or is
  deleted. The Log lists the deleted names; git history keeps them.
- `tools/check_public_content.py` refuses an added line with U+2014, with a
  test case next to the existing ones.
- No tracked text file under `tasks/`, `docs/`, `tools/`, `AGENTS.md` and
  the READMEs holds U+2014 (a colon, a comma, a full stop or parentheses
  take its place; the meaning stays).
- Task 225's header is valid.
- Optional, only if it stays under 15 lines: the owner shot's two detached
  writer threads (`ce/src/devtools/owner_shot.cc:465`, `:810`) are joined
  at exit, so F12 followed by an immediate quit leaves a whole PNG.

## Out of scope

Worktrees, remote branches and the main checkout (198); `runs/` evidence.

## Log

- 2026-11-11 claude: cut from the audit.
