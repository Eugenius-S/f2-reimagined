---
id: 260
title: Verify every Abbey burnt rebar placement stands in flight
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: low
tier: 4
claude-only: no
user-check: no
depends-on: [123]
reviewed: yes
---

## Goal

Verify the three Abbey e0 burnt rebar placements that task 123 did not join to a wall plane, and determine whether they satisfy the original requirement that no rebar lies flat on a roof or floor.

## Context

- Task 123 reports 9 scenery wdbeam placements at Abbey e0, of which 6 receive a `wallCards` PLANE entry. The other three retain existing treatments: one walls-batched SKIP and two accepted fits.
- The committed Abbey pairs show the joined cases at multiple fly turns and zoom, but do not isolate these three exceptions. The task's acceptance covers every affected piece.
- This is an evidence and diagnosis task. Do not change the exporter rule unless a matched capture shows a specific remaining failure.

## Acceptance

- Identify the map, elevation, tile, art and baked/export treatment for each of the three Abbey placements.
- Capture matched before | after fly views at turns 0, 90, 180 and 270, with one close view, for each placement; include the game view if the rebar's placement is unclear.
- Record the build/cache identity, camera turns, zoom, and relevant sidecar mode in JSON sidecars. Keep committed images under 1 MB each.
- Confirm whether every piece stands on or continues its wall, or record the exact placement and view where it lies across a roof or floor. If a defect is visible, create a narrowly scoped cause-based fix task and link it here.

## Out of scope

New art, engine changes, unrelated ruin fixes, and rechecking placements already covered by task 123's joined pairs.

## Log

### 2026-11-12 gpt

Done: identified Abbey e0 tiles 9309 (WDBEAM04.FRM, object 948, baked SKIP),
11703 (WDBEAM01.FRM, object 896, accepted fitted card) and 11709
(WDBEAM01.FRM, object 890, accepted fitted card). Four close fly views at
turns 0, 90, 180 and 270 have matched before | after sheets and JSON sidecars
under `runs/260-abbey-rebar-exceptions-evidence/`. A game-view reference is
included for each. All three treatments match before and after task 123; the
rebar stands with the vertical ruin structures in the inspected views, with
no piece lying across a roof or floor. No fix task indicated.
Next: owner can review the three committed comparison sheets.
Check: background captures via `tools/dev-snapshot.ps1` on the pre-123 cache
from `002b6971d^` and the current cache; every sidecar reports Abbey e0,
requested view tile, full `scene-gl`, and the same executable SHA-256.
Capture note: CE snapshot `camera` reports the base static pose; each task 260
comparison JSON records the requested fly turn beside the source snapshot
sidecars.

### 2026-11-12 gpt review

Follow-up created after reviewing task 123: the saved pairs cover joined rebar but not the three Abbey exceptions reported in its Log. Their actual orientation is unverified; this task makes no claim that they fail.
