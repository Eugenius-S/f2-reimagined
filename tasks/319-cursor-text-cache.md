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

Inventory with SharpText on: four successive true logical cursor placements sampled in the first presented frames. The current crops changed by 453, 496, 464 and 460 pixels against a hidden-cursor baseline; every other crop fully restored to its baseline on each move. Local 3D stayed active. This covers the stock Klamath stats panel at the test resolution, not every cursor animation or the owner save.

Dialogue sweep: repeated the test with four actual logical placements
over Sulik's reply text in separate first-present frames. Current crops
showed 806, 808, 1271 and 808 changed pixels against the no-cursor baseline.
All other crops were restored exactly before the new frame presented.
Local 3D stayed active. This checks the reported trail type on this
native text fixture, not a physical cursor driver or every font layout.
