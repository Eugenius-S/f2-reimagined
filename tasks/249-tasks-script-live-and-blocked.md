---
id: 249
title: The board script never hands out a live task and shows the blocked ones
milestone: M1
status: todo
owner: any
suggested: glm
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`tools/tasks.ps1 -Next <agent>` offers only work that is really free, and
the blocked tasks stop being invisible.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problem 10).
  Stands alone: no open task covers the script.
- `tools/tasks.ps1:48-63` looks for the branch by the exact name
  `task/<file name>`. Task 186 is `doing` on main and its work is on
  `task/186-installer-cleanup` (PR #401), so the script calls it stale and
  both `-Next glm` and `-Next gpt` print "continuing a stale task of claude".
- Lines 101 and 125 take only `todo`; `blocked` (8 tasks on 2026-11-11) is
  never offered and never listed with its reason.

## Acceptance

- A task counts as taken when any `origin/task/NNN-*` branch has a commit in
  the last three days, or an open PR's title starts with `NNN:` (one
  `gh pr list` call for the whole board; without `gh` the script says so and
  uses the branches only).
- With PR #401 open, `-Next glm` does not print 186.
- The plain board prints the blocked tasks in one block at the end, each
  with the first Log line that holds the word `Blocked` or `blocked`.
- A status outside `todo | doing | done | blocked` prints one warning line
  naming the file.
- The stale and taken decision is one function with a self-check: the script
  run with `-SelfTest` feeds it four made-up rows (live branch under another
  slug, open PR, a three-day-old branch, no branch) and fails on a wrong
  answer.

## Out of scope

Changing task statuses (248); the Polka board script outside the repository.

## Log

- 2026-11-11 claude: cut from the audit.
