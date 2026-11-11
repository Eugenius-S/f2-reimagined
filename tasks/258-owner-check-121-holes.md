---
id: 258
title: Owner visual check of task 121 hole fills before references are accepted
milestone: M1
status: todo
owner: owner
suggested: any
priority: low
tier: 4
claude-only: no
user-check: no
depends-on: [121]
reviewed: no
---

## Goal

Record the owner's visual pass on task 121's filled holes, especially fence rails, shrubs, shelves, cactus gaps and the vault elevator shaft, against the saved before | after pairs before treating the retaken regression references as visually accepted.

## Context

- Task 121 acceptance says regression references update after the owner's visual pass. Its landing Log says they were retaken "after the owner's pass", but the merged PR #180 and task record contain no owner screenshots, linked note or other evidence of that inspection.
- PR #180 has six before | after pairs and a fly-tour sheet under `runs/121-holes-filled-everywhere/`; the fill previously smeared open wooden fence-rail gaps, fixed by limiting the band fill to 4 texels.
- Tasks 175-182 are per-city fly-throughs and do not record this specific fill review.

## Acceptance

- The owner records whether the saved pairs show only acceptable pinholes/slits filled with edge colour, with attention to fence-rail gaps and the noted vault-elevator smear.
- If the owner identifies an unwanted fill or remaining visible hole, attach the exact image/region and create or fold a cause-based fix task before updating references.
- Record the owner's result and evidence path in this task's Log; keep task 121's `user-check: yes` semantics until then.

## Out of scope

Changing the hole-fill algorithm or retaking regression references without an identified visual decision.

## Log

- 2026-11-12 gpt review: Follow-up created because the landing record asserts an owner pass but provides no evidence of it. No claim is made about whether the owner actually viewed the frames.
