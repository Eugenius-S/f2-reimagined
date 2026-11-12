---
id: 189
title: Walls across every map, one rule per PR
milestone: M1
status: doing
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every wall in every map stands right in 3D: faces, tops, corners, posts, edges, see-through. Task 107 (the painted corner posts, #170) is part of this batch.

## Context

- Rule 1 folded in (2026-11-04 sweep): task 107, a corner post's card
  splits at its own axis, each half on the face it stands before. Its file
  `tasks/107-*.md` keeps the full analysis; gpt's draft #170 (stale since
  2026-11-01) is the start: continue from its branch.
- The owner, 2026-11-04: geometry is fixed by kind, one class of objects
  across every map per task and per PR, never map by map.
- Sources: the finds of this kind in the area tasks' Logs (136, 158-171),
  the owner's screenshots (183) and the city reviews (175 on). Move each
  find here as a numbered line, then fix them by one rule.
- One run of this task is one PR in size: take the most visible cause, fix
  it everywhere, merge on green CI, then the next cause. The task stays
  open while finds of its kind keep coming.
- Speed and geometry work touch the renderer: never run this alongside a
  speed task (172, 156, 157).

## Acceptance

- Each PR: the rule it adds, before | after crops on at least three maps,
  ctest and the regression.
- The Log lists every find of this kind marked fixed (PR) or open.

## Out of scope

Other kinds of objects (their own batches), per-location fixes from the
owner's shots that are not of this kind.

## Log
- Folded Task 107's painted corner-post rule into this task. Added geometry detection that splits a terminal post strip at its axis and places it on the perpendicular face. Added exporter tests for row detection, owner matching, and split geometry.
- Full regression found intended painted-post changes at Broken Hills entrance (76 pixels) and jail (60 pixels). Updated only those two references and reran both spots; each passed with zero changed pixels. See `runs/189-batch-walls/brokenhills-post-regression-diffs.jpg`.
- Matched current-main captures show 104 changed pixels at Modinn tile 19531. The same-owner candidates at Kladwtwn tile 25874 and GSTFARM tile 12439 show zero changed pixels in both tested views. The exact folded acceptance view at bathhouse tile 18930, zoom 2.5, turn 40, hours 8 and 17 also shows zero changed pixels. See the three-map, bathhouse, and Modinn sheets with JSON sidecars.
- The 24-view fly-tour control set reports zero changed control views. See `runs/189-batch-walls/fly-tour-changed.jpg`.
- Build succeeded. CTest: 20/20; exporter tests: 128/128; regression unit tests: 12/12. Full regression passed after updating and rerunning the two Broken Hills references.
- Acceptance remains open: the expected visible post effect is not present in the exact bathhouse acceptance view or the tested Kladwtwn/GSTFARM candidates. Modinn changes are visible, but this does not establish the rule across maps. PR remains draft. Next step is to diagnose visibility/face selection for these locations before claiming completion.
- 2026-11-12 task 248 audit: missing implementation evidence for the painted-post rule at the required bathhouse, Kladwtwn, and GSTFARM views; diagnose face selection and make the general rule visible across maps before closing.
