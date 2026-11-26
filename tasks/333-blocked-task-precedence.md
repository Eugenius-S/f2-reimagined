---
id: 333
title: The task picker respects a blocked card on main
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [249]
reviewed: no
---

## Goal

Keep a task blocked on main unavailable in the board and in `-Next`, even when
an old task branch says it is in progress.

## Context

Follow-up to [249](249-tasks-script-live-and-blocked.md). On main
`9f54f368`, task 316 is blocked, but `tools/tasks.ps1 -Next gpt` selects 316
as the agent's own doing task. The older exact-slug branch at `b21c78cc`
says `doing`. The resolver reads the branch copy before it evaluates the
status, so it discards the blocked status and Log from main.

## Acceptance

- A main `blocked` card keeps its status and Log when a branch says `doing`.
  `-Next` never selects it.
- Keep the existing handling for main `done` and for a task stopped as blocked
  on its own branch while the main card is still open.
- Cover branch/main precedence in the task-picker check and verify the live
  `-Next gpt` command no longer returns task 316 on this main.

## Out of scope

Changing release gates, reopening blocked tasks, rewriting branch history, or
changing Polka's priority and ownership controls.

## Log

- 2026-11-27 gpt: reproduced on current main while preparing the next batch.

- 2026-11-27 gpt: Main blocked and done cards retain their full body; open cards still use their branch. SelfTest covers both terminal statuses and open-to-blocked/doing precedence. `-SelfTest` passed; `-CheckMetadata` passed (326 tasks, 225 legacy gaps); live `-Next gpt` selected stale task 186, not 316. Independent review remains.

- 2026-11-27 gpt: Review follow-up shares the card-source resolver between board and SelfTest, including absent branch content and no branch read for blocked/done. SelfTest and metadata passed; live `-Next gpt` still selected stale task 186. Independent review remains.
