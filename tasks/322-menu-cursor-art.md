---
id: 322
title: Preserve the cursor over menu wallpaper
milestone: M1
status: todo
owner: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

From overnight queue #317. Preserve software cursor opaque pixels when owner RGBA art replaces the indexed background; composite before the full-frame OpenGL upload. Keep transparent cursor pixels transparent to the wallpaper. This is separate from cache invalidation in #319.

Windows acceptance: move across wallpaper, existing foreground strips, and buttons, with sharp text on and off; there should be no missing cursor pixels, solid box or trails. Compare startup menu and death screen with current assets. Pure cursor clipping tests pass; runtime validation pending.

## 2026-11-23 Windows evidence

The official 3440x1440 menu showed its art with a visible software cursor in the sampled position. A full sweep over the reported stripe and the death screen is still separate coverage.

[Native acceptance checks](../runs/325-night-checks/README.md).

Follow-up: six sampled positions at 3440x1440 / 2x with SharpText on changed only the 36x36 cursor footprint versus the hidden cursor. The first sample also passed with SharpText off; death art and an exhaustive sweep remain separate.

### 2026-11-23 release reconciliation

The main-menu cursor fix shipped in `039d25da3`; six native wallpaper spots passed. Remaining: sweep the reported stripe and buttons with SharpText on and off, and run the separate death-screen comparison.
