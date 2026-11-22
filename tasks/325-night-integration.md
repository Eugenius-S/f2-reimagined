---
id: 325
title: Verify the combined November 22 bugfix candidate
milestone: M1
status: done
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

Combine the separately self-reviewed and tested proposals #554 through #561 so the Windows CI compiles the actual product combination. Do not merge the source PRs independently as a second copy.

## Windows checks

1. Exit cues and actual transitions, separate checks: Arroyo and random encounter; walk to the exit with and without a screenshot; classic control, rotated camera and developer flight.
2. Diagonal and backward approach to authored EDG bounds; preserve the widened scroll range and the straight cutoff.
3. F12 in main menu, options, dialogue, Pip-Boy, 3D and world map. Verify exactly one PNG and JSON per press. Region marking before and during combat remains separate.
4. Cursor across main menu wallpaper and dialogue text with sharp text on and off, including moving away and cursor animation.
5. Clock ladder at arvillag tile 25115; compare floor, head and torch against the released build, plus a non-Arroyo map. Compare with direct (not baked) ground lighting if needed. The grade is provisional.
6. Window mode, physical resolution, preservation of local 3D and pointer alignment; local world map remains 2D. VSync setting survives restart and unsupported sync does not block startup.

Run only in the isolated test game. See the exact evidence below. Pending separate repros: magenta bridge around capture, pot PID 243, silhouette cleanup, fullscreen fades and soft shadow calibration.

## 2026-11-23 Windows evidence

The combined patch is merged and published. The individual acceptance gaps are tracked on 317 to 324, the original 316 release follow-ups, and the evidence table. Pot-rim PR #563 is published. Outline PR #564 was merged, verified natively, and dispatched for a separate release.

[Native acceptance checks](../runs/325-night-checks/README.md).
