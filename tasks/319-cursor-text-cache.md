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
