---
id: 257
title: Show a normally visible Klamath point light beside its cutout wall pixels
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: low
tier: 1
claude-only: no
user-check: no
depends-on: [099]
reviewed: yes
---

## Goal

Capture a Klamath barrel or slime point light at normal ambient settings where
the pre-discard wall-normal fix visibly changes the light along nearby cutout
wall pixels.

## Context

- Task 099's KLATRAP barrel before/after pair changes only 4 pixels with the
  dev-only ambient override 16384; its evidence says normal ambient 1.0 gives
  no measurable difference.
- Task 099 is reviewed and merged. This follow-up asks only for stronger
  normal-settings visual evidence, not a shader change.

## Acceptance

- In `runs/257-klamath-point-light-normal-settings-evidence/`, provide matched
  3D before/after frames from the same Klamath map, elevation, tile, camera,
  resolution and scale, at normal ambient 1.0 and with the visible barrel or
  slime point light enabled.
- Record the decoded RGBA changed-pixel count and sidecar metadata.
- Inspect a crop of changed pixels. Changes should follow nearby cutout wall
  contours without isolated new dark or bright specks.
- If no normal-settings Klamath view shows a measurable difference after
  checking the barrel and slime candidates, record the tested views and
  settings; do not alter light colors or strength for this evidence task.

## Out of scope

Shader behavior, point-light colors or strength, ambient-setting changes.

## Log

- 2026-11-12 gpt review of task 099: created because the existing Klamath
  barrel pair needs ambient override 16384 and shows only 4 changed pixels.

### 2026-11-12 gpt
Done: captured matched historical/fixed shader pairs for the KLATRAP barrel and KLATOXCV slime at ambient 65536 (1.0). Both pairs have zero decoded RGBA pixel differences; metadata and full-frame sheets are in `runs/257-klamath-point-light-normal-settings-evidence/`. Visually confirmed the red-topped barrel in KLATRAP and toxic green slime plus cutout walls in KLATOXCV; scene light is enabled. No changed-pixel crop exists because the target comparisons are identical. The natural KLATOXCV ambient 28426 comparison changed 200 pixels and is documented separately, not counted as the 1.0 result.
Next: reviewer may inspect the draft PR; no merge performed.
Check: `tools/configure-build.ps1 Release`; paired `tools/dev-snapshot.ps1` captures; Python decoded-RGBA comparison reported 0 changed pixels for both ambient 65536 pairs.
