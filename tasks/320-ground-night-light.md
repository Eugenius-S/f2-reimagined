---
id: 320
title: Apply scene light to the baked floor
milestone: M1
status: doing
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

The night report is a symptom. The verified defect is that the half-resolution floor light pass called the shader in light mode zero, so it baked white instead of the live ambient and local lights whenever the bake ran.

Windows graphical acceptance: compare Arroyo day, dusk, and midnight at the same fixed camera; compare with F2CE_NO_BAKE for diagnostic parity and verify a local source still lights nearby floor. Compare a non-Arroyo exterior. Keep separate from global night grade and composite character sprites.

## 2026-11-23 Windows evidence

Published in #562. Fixed-camera shadow-active Windows controls in the current build support the original fix: Arroyo baked ground darkens with the direct lighting control, and Klamath direct ground also darkens overnight. The test kept the torch and UI visible. Exact conditions and crop values are in the evidence table.

[Native acceptance checks](../runs/325-night-checks/README.md).
