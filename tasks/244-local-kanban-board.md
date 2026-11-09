---
id: 244
title: A local kanban page shows the whole board at a glance
milestone: M1
status: done
owner: claude
suggested: claude
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner opens one local page and sees the whole project: every task as a
card in columns by state, epics with their progress, filters by tag and
priority, a search. Like the Jedi Academy board the owner showed on
2026-11-09 (columns: epics, under discussion, queued, in work, in-game
check, owner review, done).

## Context

- The source of truth stays `tasks/*.md` (front matter: id, title, status,
  owner, priority, milestone, depends-on) and `tasks/inbox.md`;
  `tools/tasks.ps1` already reads them. The page is generated from them, not
  a second copy to keep in sync.
- Local only: a static HTML file the owner opens in a browser, rebuilt by one
  command (or on every board merge). No server, no account.
- Card: number plus a plain Russian name, owner (claude, gpt, glm), priority,
  links to the task file and its PR. Epics: the milestones or task groups
  with "parts done of total".

## Acceptance

- One command builds the page from the repo; it opens offline.
- Every task on the board is on the page in the column of its status;
  done tasks fold away.
- Filters by owner, priority and milestone; a search by number and words.
- The owner checks it and says the picture is clear.

## Out of scope

Editing tasks from the page; drag and drop that writes back.

## Log

- 2026-11-09 claude: cut at the owner's request (screenshot of the Jedi Academy board); nothing built yet.
- 2026-11-09 claude: done another way, by the owner's choice: his Polka 3.8 board (a static page with columns, priorities, links and search) in the folder `!F2R board` next to the checkouts, outside the repo. `update_board.py` there rebuilds its data from `tasks/*.md` on origin/main (242 cards, plain Russian texts in `ru-tasks.json`) and sends the owner's edits back as a board PR (changed priorities into the task files, new cards into the inbox), through its own worktree `f2r-board-sync`. The rule for agents is in AGENTS.md (Tasks). No timer, by the owner's word: the sync runs after a task merge and when he adds cards. Not done: the order of cards inside a column is not sent back.
