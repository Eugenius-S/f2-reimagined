---
id: 322
title: Preserve the cursor over menu wallpaper
milestone: M1
status: blocked
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
### 2026-11-26 gpt

Check additional native cursor positions and capture the limits of the shipped startup and death background paths.

### 2026-11-26 gpt: bounded acceptance stopped

Done: inspected the shipped composition path in `ce/src/svga.cc`: it preserves opaque cursor cells while replacing masked wallpaper pixels. The archived `snapshot_rows_tests.exe` returned exit 0, including the cursor bounds, pitch, transparent-cell and null checks in `ce/tests/snapshot_rows/snapshot_rows_test.cc`. This is a helper test, not proof of a presented menu frame. The archived test executable SHA-256 is `84dd4d7e92b69b6064bf4c3c21de2fca25437eca0196273466ea9b0e67b41811`; its exact build provenance was not established. No new defect is claimed.

Blocked: the stock `--dev-menu-snapshot` shows the cursor and immediately captures and exits before the menu input loop; it has no cursor position or sweep control. The existing `tools/game-input.ps1` selects the first matching game process or window and cannot safely target only an isolated test process; it also has no move-only command. No existing death-screen cursor fixture was found. No background captures were attempted because neither path can produce the required controlled positions safely. The six historical wallpaper samples remain valid, but do not establish stripe, button edge, moving-away or death-screen coverage.

Next: provide a narrowly scoped in-process menu/death fixture that moves the software cursor to specified logical coordinates, presents successive frames over the reported stripe and button edges and after moving away, and captures the actual presented frames with SharpText on and off. Run against the exact Release `5e8738eab40dc770617f561a14230db35d6f8c6c` (EXE SHA-256 `c1cc0353f123be55b82edd207265bf9315e51f59692dcbc5a5f990aa68334ad7`) in an isolated copy with packaged wide art. For death, supply an isolated reachable death fixture with current art. Keep this task open until those checks pass.

### 2026-11-27 gpt: instrumented button-edge attempt

Built an instrumented Windows Release from branch `edc2f16831b7e701aff1fa72c6f746452eb08662` (code based on `5e8738eab`, later commits are task documentation). The uncommitted QA-only patch, raw frames, isolated donor and build log remain under `build/322-qa/`; the instrumented EXE SHA-256 is `c1a362e3932399be69a7815ff59cb198616c42ac680b3195d2d6e76bd219d8c0`, not the stock binary. Patch positions the engine cursor in the menu and calls `renderPresent` plus `capturePresentedFramePng`. Both runs used the packaged wide menu art, offscreen window at 3440x1440, logical 1720x720, 2x. Metadata reports SharpText setting off and on respectively, with the overlay inactive in the menu.

The captured presented frames lagged the requested sequence by one present: the requested edge frame equals the hidden baseline, and the requested away frame instead contains the cursor at logical (98,39), overlapping the first button edge. Both modes produced the same pixels; 856 physical pixels differ from the hidden frame in bbox (196,78)-(232,114), exclusive right/bottom. [Crops](../runs/322-menu-cursor-art/button-edge-qa.jpg) show that sampled appearance only. This does not establish an expected cursor pixel mask, movement cleanup, the stripe, the second position, or death art; no regression or pass for the whole case is claimed. The second actual cursor position was not captured, so the fixture must wait for and verify the subsequent presented frame and retain the same-frame cursor snapshot for a pixel comparison. Do that in a new isolated attempt, and separately provide a reachable death fixture. No stock or production code was changed.

### 2026-11-27 evidence archive

The six original captures and their frame-order notes are in the separate
[322 evidence ZIP](https://github.com/Eugenius-S/f2-reimagined/releases/download/evidence/322-cursor-qa-c2f5a723.zip).
Downloaded and checked SHA-256
`c1d78168f66456acac24a4a695495b6892c8cbff9d81cb22843231da357d51dd`.
The crop sheet remains the quick view; see the limits in the attempt above.
