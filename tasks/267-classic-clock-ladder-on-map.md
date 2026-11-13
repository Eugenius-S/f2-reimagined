---
id: 267
title: The classic clock ladder captures the map it was asked for
milestone: M1
status: todo
owner: gpt
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

`tools/dev-snapshot.ps1 -ClockLadder -Classic -Map <map>` saves the original 2D picture of that map at each hour of the ladder.

## Context

Draft PR #472 adds the mode, but two runs captured ARTEMPLE instead of `city1`: the ladder starts before the requested map is loaded. Finish that branch. It unblocks 262 and 263, which need matched 2D day and night frames.

## Acceptance

- One run on `city1` and one on an `encspecial` map: every sidecar names the requested map and elevation, and the frames differ between 22:00 and 09:00.
- A map that never loads fails the run with a clear message instead of capturing another map.
- ctest passes; the 3D clock ladder still logs `CLOCK_LADDER PASS` on kladwtwn.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.
