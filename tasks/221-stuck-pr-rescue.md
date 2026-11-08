---
id: 221
title: Every stuck PR either lands in main or is closed with the reason
milestone: M1
status: done
owner: any
suggested: glm
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

The 2026-11-08 audit found 18 open PRs, 16 of them drafts, most with green
checks, while their tasks went back to `todo` on main. Finished work sits
outside main and the owner never gets it.

## Context

The 2026-11-04 strategy was "merge on green CI", yet the PRs below stayed
drafts while agents moved on. Some are superseded by later merges (#329
covers part of #227); #320 and #336 carry board ids that clash with main.

## Acceptance

- Each PR in the list below ends merged into main, or closed with a
  one-line reason and a link to what replaced it. Nothing stays a draft.
- Each merged code PR passed ctest and one launch check on the demo copy
  before the merge; main stays playable after each.
- Each PR's task carries its real status (done, or todo with what is left).

## Out of scope

New features or polish beyond what a PR already holds; the owner's
`game/` folder; PRs opened after this task was cut.

## How, per PR

One PR at a time, in the order below. For each:

1. Check out its branch, merge fresh `origin/main` into it, fix conflicts.
2. Compare with main: if main already has the same result (another PR did
   it), close it with a comment naming that PR, set the task done or
   folded, and go to the next.
3. Otherwise build Release, run ctest, run one `tools/dev-snapshot.ps1`
   launch check on the demo copy (AGENTS.md: main must always be playable).
4. Green and the game starts: mark it ready, squash-merge, set the task's
   status (done, or todo with what is left in the Log). Red or it breaks
   the game: one fix attempt; if that fails, close it with the reason and
   keep the task `todo` with the finding in its Log.

## The list, by value

1. #336 157/219 ground light bake, sharp-text cache, object cards (it has
   no `tasks/219-*.md`: add the file before merging).
2. #320 202 the sea fill past a water corner. Its task files 202, 203, 204
   and 205 collide with other tasks of those ids on main: renumber them to
   222, 223, 224 and 225 before merging.
3. #272 173 3D never turns off on local maps.
4. #300 149 combat overlays follow the 3D view.
5. #296 135 the 3D view stays on in combat (task notes only).
6. #289 187 RP maps in 3D; #284 185 RP sfall gap list.
7. #302 189 walls (it carries 107, the corner post split, as its first
   rule); #308 191 furniture and props.
8. #310 193 UI text layout; #292 130 the options screen and cursor speed.
9. #206 140 whole letters in dialogue answers.
10. #227 156 live display change: #329 (209/210) already applies the
    display mode live; land only what #329 does not cover.
11. #221 147, #220 146, #208 143, #189 111, #298 128: diagnostics and task
    notes; merge the notes, close the rest.

## Log

- 2026-11-08 claude: cut in the project audit.

## Log

- 2026-11-08 glm: the sweep is complete, no open PRs remain. Merged: #320 (202, tasks 202-205 renumbered to 222-225), #272 (173), #300 (149), #296 (135 notes), #289 (187), #284 (185), #220 (146), #206 (140), #221 (147), #208 (143 notes), #189 (111 notes), #298 (128 notes), #302 (189 walls, brokenhills references retaken on the merged tree, full regression 0 px elsewhere), #292 (130), #310 (193 part A, full regression 0 px). Closed with reasons: #308 (191: the light rule rewrites unaccepted enclave pixels, full regression evidence in the task Log), #227 (156: #329 covers window mode live, the live resolution/scale transaction is runtime-blocked, task 209 owns the slice; the branch notes preserved under task 156). #336 was skipped by owner order and then merged by the owner; its launch-check finding (static wall draw GL error) is on the PR and did not reproduce on final main (ed939119b, build + ctest 20/20 + launch check green).

- 2026-11-08 claude: checked. No open PRs; every one merged or closed with a reason. Board statuses match main except 140, set to blocked as #206's merge says (it waits for a dialogue fixture). Main after the sweep and after #347/#349: Release, ctest 20/20, launch check green.
