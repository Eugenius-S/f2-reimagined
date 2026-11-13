---
id: 269
title: The snapshot sidecar says which shadows, clouds, sun and glow were really on
milestone: M1
status: blocked
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

### 2026-11-13 gpt review
Blocked: the earlier completion claim is withdrawn. Static inspection confirms the synchronous snapshot draw sets the shadow uniforms before image readback, but the first implementation reported zeroes for Classic/non-hero fields, treated a configured glow pass as applied, and did not check errors after uniform reads. The revised code emits JSON null for unsupported or failed reads, scopes point-light reads to an active hero-light draw, and validates nullable values. Regression unit tests cover missing state plus 2D and non-hero unknown schemas.
Evidence limit: existing matched captures are real and their sidecars report cloud shadows off/on, but both frames have a large white ground defect (877,587 pixels, 42.3% of each frame are near-white in both images). A diff counted 250,743 changed pixels at channel threshold 2, including 6,415 in the white region; the other changes do not establish a cloud-shadow change on a valid ground surface. No further captures were taken because the known white-ground reproduction is task 270 and the pair's visual acceptance is invalid.
Not checked: runtime output from the revised sidecar path, Classic/non-hero runtime captures, and the exact applied glow result. `glowActive()` is only the configured gate; reporting active glow as null is intentional because capture success is returned inside the out-of-scope renderer. A valid cloud A/B pair likewise needs a non-white ground capture. Keep PR #503 draft; do not merge until those runtime facts are available.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release` succeeded; `ctest --test-dir build --output-on-failure` passed 27/27; `py -3 -m unittest discover -s tools/regression -p 'test*.py'` passed 15/15. The first configure attempt caught an incorrect direct `glGetError` link; code now uses the renderer's loaded GL function pointer. No runtime capture, perf-check, CI, or merge was attempted.
