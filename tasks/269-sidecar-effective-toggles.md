---
id: 269
title: The snapshot sidecar says which shadows, clouds, sun and glow were really on
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every devshot JSON records the effective state of sun shadows, cloud shadows, point lights, glow and ambient at the moment of the capture, read from the renderer, not from the command line.

## Context

143 (soft cloud edges) is blocked because six captures could not prove which toggles were in effect (PR #470). The same doubt sits in 256, where the 3D frame came out with white ground.

## Acceptance

- The sidecar has one object with the five values; a self-check run with `-CloudShadows` off and on shows the field change and the picture change together.
- A unit or script test reads a sidecar and fails when the field is missing.
- ctest passes.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.

### 2026-11-13 gpt
Done: devshot sidecars now record the renderer's sun-shadow, cloud-shadow, point-light, glow, and ambient state in `effective_render_state`. Added strict regression validation and a missing-field test. The matched captures are in `runs/269-effective-toggles/`.
Next: controller reviews and merges draft PR #503; do not merge or run post-merge Rule 1 here.
Check: Release configure/build succeeded; `ctest --test-dir build --output-on-failure` passed 27/27; `py -3 -m unittest discover -s tools/regression -p 'test*.py'` passed 13/13. `tools/dev-snapshot.ps1` off/on captures recorded cloud_shadows false/true and changed 323,995 pixels.
Not checked: CI; raw JSON sidecars remain local under `build/task269/` and are not committed.
