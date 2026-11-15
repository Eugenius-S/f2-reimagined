---
id: 248
title: The board tells the truth, done work is closed and evidence debts move to the owner's fly-through
milestone: M1
status: done
owner: any
suggested: claude
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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
Check: Pre-change `tools/tasks.ps1` reports status anomalies on 207 and 145; `tools/tasks.ps1 -Review` lists 65 tasks. PR/branch lock checks found no 248 PR or remote task/248 branch.
Blocker: Acceptance asks to update AGENTS.md, but this work is explicitly limited to task files and the board. Record this conflict in the final Log and leave AGENTS.md unchanged.

### 2026-11-11 gpt: audit and scope blocker
Historical result, superseded by the 2026-11-12 audit below. The prior 65-row review queue and invalid 207 status reflect the pre-change main board.

Done: Rechecked task states against their Logs and mainline merge evidence. Closed tasks 127, 135, 147, and 149 where implementation is in main and only owner live capture remains; moved those captures to task 176. Kept 111 todo because the live state is still undiagnosed. Returned 140 and 145 to todo with the missing fixture or implementation stated. Normalized invalid status 207 from open to todo. Left task 146 untouched because active PR #408 owns it.
Next: Add the owner's captures to task 176; task 146's live combat fixture is active. Independently review old done PRs that changed regression references or engine file formats.
Check: At that historical point, `tools/tasks.ps1 -Review` listed 65 tasks before and after the unmerged changes. The post-change script read `origin/main`, so it showed the pre-merge board and status 207 as open. Local header edits passed `git diff --check`. `update_board.py` reported 252 tasks and no new Polka cards; `check_public_content.py origin/main` passed.
Historical blocker: At that point the requested `AGENTS.md` change and the PR audit were still incomplete. This was resolved by the 2026-11-12 work recorded below.

### 2026-11-12 gpt: round-two acceptance and PR audit

Done: Added the two-background-failure rule to `AGENTS.md`; moved task 127 to
`done` because its FPS display, live fly overlay, frame log, and baselines are
in main, with the remaining F8 owner demonstration assigned to task 176.
Changed invalid task statuses 145 (`reopened`) and 207 (`open`) to `todo`.
Task 145's Log names the missing Artemple 21:9 edge and tree implementation.
Task 111, 143, and 189 remain `todo` because their required code or diagnosis
is still missing. Task 130 remains untouched under active PR #415; task 146
remains untouched under the other agent's active PR #408. Files 135, 140,
147, and 149 were restored exactly to fresh main, preserving glm's newer
logs and `blocked` statuses as the owner's explicit exception to this audit.

Review audit: the pre-change `-Review` output had 65 IDs. The status-setting
commit and its PR were established from `git log -S 'status: done'` for each
task file, then associated PR file lists were read with `gh pr view`. For
board/docs closure PRs, also read the implementation PRs linked from each
task's Log or history. Set `reviewed: yes` on 51 tasks. Kept `reviewed: no`
for tasks 058 (#114), 067
(#121), 096 (#142), 099 (#140), 100 (#147), 121-125 (#180), 170 (#291),
171 (#312), 174 (#274), and 183 (#318), because those PRs changed
`tools/regression/reference/` files that remain unreviewed. No task lacked an
identifiable status PR. PR #317 is also part of task 183's reference refresh.

Checks: `tools/tasks.ps1 -SelfTest` passes five cases, including blocked task
248 with open draft PR #414; `git diff --check` passes. Before review IDs:
017, 023, 048, 050, 051, 058, 062, 067, 082, 089, 091, 096, 099, 100,
101, 102, 103, 109, 110, 116, 120, 121, 122, 123, 124, 125, 126, 129,
132, 137, 138, 139, 144, 168, 170, 171, 174, 183, 185, 187, 202, 203,
204, 205, 206, 208, 209, 210, 213, 214, 215, 216, 217, 218, 222, 223,
234, 235, 236, 237, 238, 239, 240, 243, 244.
After review, the expected remaining IDs are 058, 067, 096, 099, 100,
121, 122, 123, 124, 125, 170, 171, 174, 183. The final `-Review` run
against a temporary local candidate checkout is recorded in the PR body.
Before `-Next gpt` selected task 156. Against the local candidate it selected
task 156 both before and after the change, `tasks/156-live-display-change.md`.

Not checked: owner-only capture requests transferred to task 176; active PR
work in tasks 130 and 146; independent review of the retained reference PRs.
