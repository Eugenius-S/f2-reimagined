---
id: 305
title: GPT owns the project strategy, board and follow-up reviews
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The current rules name one project lead, GPT, and carry the owner's model routing without changing the strategy or merge gates.

## Context

- Standalone handover requested by the owner on 2026-11-15, not another board-tool task (248, 249, 300).
- `AGENTS.md:11-24,195-207,252` still gives Claude strategy, Polka and follow-up reviews; `CLAUDE.md:3-7` gives it the lead's session routine. The team split in `docs/2026-10-19-roadmap-design.md` must agree too.
- `AGENTS.md:14` requires exactly one Luna for code. The owner's current routing is: main Sol (`gpt-6.1-sol`) high; brainstorming Astra (`gpt-6-astra`) high; code Sol, Terra or Luna at an effort suited to the work; image generation Luna (`gpt-6-luna`) low. Above Luna, read Ponytail and use full unless the owner chooses otherwise. Never silently substitute an unavailable model or change the active chat's model.
- Preserve the project's one-working-subagent limit; model routing does not create extra parallel slots. GPT keeps analysis, specs, decisions and acceptance. Claude executes critical fixes and design-heavy core work; GLM keeps the shared pool.

## Acceptance

- AGENTS, CLAUDE and the roadmap's current team split agree on GPT leadership, Polka edits/sync, task cutting, inbox folding and follow-up review. Historical Logs stay untouched.
- Move the lead's session routine into AGENTS: sync owner edits, review completed work against evidence, resolve blocked scopes, fold inbox finds, keep free work, verify Rule 1 handover/publication and both READMEs.
- CLAUDE points to AGENTS and `tools/tasks.ps1 -Next claude` as an executor. The main chat retains decisions and acceptance; model routing and Ponytail match the owner policy above.
- The shared `owner: any` pool, renderer exclusivity, green-CI merge gate, owner-game safety and Rule 1 remain intact. Check the diff and active role references in these three documents for contradictory leadership or obsolete Luna-only routing.

## Out of scope

Changing the roadmap's feature priorities, code, CI, task picker or historical reports. Foundation remains disabled.

## Log

- 2026-11-15 gpt: cut from the owner's lead handover. One bounded rules PR; no approval gate added.
