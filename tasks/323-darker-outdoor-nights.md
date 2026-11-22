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
