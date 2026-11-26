---
id: 305
title: GPT owns the project strategy, board and follow-up reviews
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

The current rules name one project lead, GPT, and carry the owner's model routing without changing the strategy or merge gates.

## Context

- Standalone handover requested by the owner on 2026-11-15, not another board-tool task (248, 249, 300).
- At handover, AGENTS and CLAUDE gave Claude the lead routine; the roadmap team split also needed alignment.
- The 2026-11-24 owner policy supersedes earlier model versions: verify the newest available role-family identifier for new work, use Standard, and do not substitute silently. Sol High leads strategy, Astra High handles deep analysis, Terra High handles principal implementation and separate independent ordinary review when required, and Luna uses xhigh for code or checks and low for prepared image prompts. Preserve active jobs.
- Run one worker at a time with a standalone brief and no inherited conversation. Use Ponytail full for coding and ponytail-review for acceptance when applicable; Luna is exempt. Use TryHard finish workflow for substantial work with the existing reviewer and release gates. GPT leads; Claude executes assigned and design-heavy work; GLM keeps the shared pool.

## Acceptance

- AGENTS, CLAUDE and the roadmap's current team split agree on GPT leadership, Polka edits/sync, task cutting, inbox folding and follow-up review. Historical Logs stay untouched.
- Move the lead's session routine into AGENTS: sync owner edits, review completed work against evidence, resolve blocked scopes, fold inbox finds, keep free work, verify Rule 1 handover/publication and both READMEs.
- CLAUDE points to AGENTS and `tools/tasks.ps1 -Next claude` as an executor. The main chat retains decisions and acceptance; model routing and Ponytail match the owner policy above.
- The shared `owner: any` pool, renderer exclusivity, green-CI merge gate, owner-game safety and Rule 1 remain intact. Check the diff and active role references in these three documents for contradictory leadership or obsolete Luna-only routing.

## Out of scope

Changing the roadmap's feature priorities, code, CI, task picker or historical reports. Foundation remains disabled.

## Log

- 2026-11-15 gpt: cut from the owner's lead handover. One bounded rules PR; no approval gate added.

- 2026-11-26 gpt: aligned active leadership, session routine, routing and review guidance across AGENTS, CLAUDE and the team split. Check: documentation diff, active role references and task metadata; no code or game checks required.
