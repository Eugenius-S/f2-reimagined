---
id: 323
title: Lower unlit outdoor ambience at midnight
milestone: M1
status: doing
owner: gpt
priority: normal
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

Separate visual tuning from the floor lighting defect in task #320. Lower 3D outdoor ambient by 26% at full night, smoothly with the existing moon's dusk and dawn timing. Do not modify native map lighting, indoor scenes, the UI, or point-light source intensity.

Acceptance: inspect fixed-camera 12:00, 18:30 and 00:00 with the floor fix integrated, verify daylight and interiors remain unchanged, and check readability and local pools around lamps. This tuning needs visual acceptance and should not ship from only a unit check.

## 2026-11-23 Windows evidence

Published with the first integration. The same Arroyo camera confirms a distinct darker 22:00 and 02:00 with readable HUD and visible fire. Daylight, another outdoor map and no-bake controls are recorded below; the complete fixed-hour visual acceptance is still open.

[Native acceptance checks](../runs/325-night-checks/README.md).

Additional exact Windows acceptance on commit 3369ef50b: a locked Arroyo
camera at tile 25115 showed [12:00, 18:30, and 00:00](../runs/325-night-checks/arroyo-noon-dusk-midnight.jpg).
The unlit ground averaged 88.10, 77.97, and 50.69 out of 255; the stone
face averaged 75.29, 64.92, and 42.32. The fire stayed visible. The paired
still images on the indoor Temple map ARCAVES e0 at tile 29107 were
pixel-identical at noon and midnight. These same-frame crops do not
measure every outdoor map or the readability of the full HUD.
