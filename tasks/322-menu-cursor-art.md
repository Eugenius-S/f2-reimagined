---
id: 322
title: Preserve the cursor over menu wallpaper
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

From overnight queue #317. Preserve software cursor opaque pixels when owner RGBA art replaces the indexed background; composite before the full-frame OpenGL upload. Keep transparent cursor pixels transparent to the wallpaper. This is separate from cache invalidation in #319.

Windows acceptance: move across wallpaper, existing foreground strips, and buttons, with sharp text on and off; there should be no missing cursor pixels, solid box or trails. Compare startup menu and death screen with current assets. Pure cursor clipping tests pass; runtime validation pending.
