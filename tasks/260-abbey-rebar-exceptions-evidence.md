---
id: 260
title: Verify every Abbey burnt rebar placement stands in flight
milestone: M1
status: doing
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

### 2026-11-12 gpt review

Follow-up created after reviewing task 123: the saved pairs cover joined rebar but not the three Abbey exceptions reported in its Log. Their actual orientation is unverified; this task makes no claim that they fail.