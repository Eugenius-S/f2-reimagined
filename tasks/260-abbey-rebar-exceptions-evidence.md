---
id: 260
title: Verify every Abbey burnt rebar placement stands in flight
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: low
tier: 4
claude-only: no
user-check: no
depends-on: [123]
reviewed: no
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

Blocked: map data identifies Abbey e0 tile 9309 / serial 109 / object 948 /
WDBEAM04.FRM (baked SKIP), tile 11709 / serial 514 / object 890 /
WDBEAM01.FRM (accepted fit), and tile 11703 / serial 506 / object 896 /
WDBEAM01.FRM (accepted fit). This data mapping does not identify the exact
sprite in the rendered frames. The original comparison captures use Solo 4;
`tools/dev-snapshot.ps1` documents that Solo removes walls and roofs, so those
frames cannot establish each rebar's relation to wall and floor. At tile
9309, turns 180 and 270 show ground and a barrel but no target rebar; turns 0
and 90 do not identify the target FID. At tiles 11709 and 11703, all four
turns show multiple similar objects and do not identify the target FID.
One full-scene, no-Solo diagnostic pass restores wall and roof geometry but
still leaves the target sprites ambiguous among clutter and occlusion. The
diagnostic frames and source metadata are in
`runs/260-abbey-rebar-exceptions-evidence/abbey-e0-full-scene-diagnostic.jpg`
and `.json`. No placement orientation is confirmed, no geometry defect is
established, and no fix task is justified from these images.
Next: obtain a capture that selects or marks each target by map serial/FID
while retaining wall and floor geometry; then repeat matched before/after
views at all required turns.
Check: visually inspected the original comparison sheets and the full-scene
diagnostic; the no-Solo pass used the background test instance, build
`5b576c093`, and isolated `build/task260-before-cache`. No matched no-Solo
after capture or owner game-view evidence was produced.

### 2026-11-12 gpt review

Follow-up created after reviewing task 123: the saved pairs cover joined rebar but not the three Abbey exceptions reported in its Log. Their actual orientation remains unverified; this task makes no claim that they fail.
