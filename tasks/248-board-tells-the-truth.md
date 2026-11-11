---
id: 248
title: The board tells the truth, done work is closed and evidence debts move to the owner's fly-through
milestone: M1
status: todo
owner: any
suggested: claude
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Eleven tasks stopped at the same place: the code is in main, the acceptance
asks for a live capture, the background harness failed twice, the task stays
`blocked` or `todo` for good. One pass gives each a real status, and the
review queue stops listing 65 tasks nobody will review.

## Context

- Found by the 2026-11-11 audit (`runs/audit-2026-11-11.md`, problems 9 and
  11). Stands alone: it is board work across many tasks, no open task holds it.
- The stuck set: 127, 130, 135, 140, 143, 145, 146, 147, 149, 189, 111.
  Example: 127's FPS counter works in main (`ce/src/settings.cc`, the
  `fps_counter` key; F8 in `ce/src/game.cc`) while the task is `blocked` at
  tier 1 over one scripted F8 capture.
- `tools/tasks.ps1 -Review` prints 65 rows, the oldest is 017. The strategy
  of 2026-11-04 made green CI the one gate.
- 246 and 146 repair the harness itself; this task does not wait for them.

## Acceptance

- Each task of the stuck set is either `done` (its code is in main and only
  a capture is missing; the missing capture is one line in the matching
  city review task 175-182, to be taken by the owner's F12), or `todo` with
  one Log line naming the code that is still missing.
- No task carries a status outside `todo | doing | done` (207 and 145 today).
- Every `done` task merged more than a week ago has `reviewed: yes`, except
  those whose PR changed an engine file format or retook regression
  references and was never read; those stay listed.
- `AGENTS.md` says what happens after the two-failure stop: the capture
  becomes a line in the owner's fly-through, the task closes on its tests.

## Out of scope

Any code change; fixing the harness (246, 146); the script (249).

## Log

- 2026-11-11 claude: cut from the audit.


### 2026-11-11 gpt: board audit in progress
Done: Reviewed the audit findings and current task logs. The stale set includes live captures and code or interaction checks; task 146 remains assigned to glm and its combat-fixture work is active.
Next: Correct statuses supported by merged main, transfer only live owner capture checks to matching city reviews, and review eligible old done tasks.
Check: Pre-change 	ools/tasks.ps1 reports status anomalies on 207 and 145; 	ools/tasks.ps1 -Review lists 65 tasks. PR/branch lock checks found no 248 PR or remote task/248 branch.
Blocker: Acceptance asks to update AGENTS.md, but this work is explicitly limited to task files and the board. Record this conflict in the final Log and leave AGENTS.md unchanged.
