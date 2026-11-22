---
id: 319
title: Refresh sharp-text cursor occlusion
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

The cursor's opaque pixels shape the glyph mask. Cache that mask only while cursor coverage is unchanged; retain cached glyphs when the cursor is outside text.

Acceptance: verify a hover across dialogue replies and then away, with OpenGL sharp text on; opaque cursor pixels cover the glyphs, transparent pixels preserve them, and no stale cursor shape remains. Repeat the main menu separately; its wallpaper layering issue is not assumed to share this cause. Pure text-layer tests pass locally. Graphical verification is pending.

## 2026-11-23 Windows evidence

Native Windows stock Sulik dialogue with SharpText on now has glyph alpha 27 to 0 under the opaque cursor and 255 to 255 under its transparent part; the test reads the exact hover PNG. The inventory trails and main-menu stripe have separate causes or remaining coverage.

[Native acceptance checks](../runs/325-night-checks/README.md).
