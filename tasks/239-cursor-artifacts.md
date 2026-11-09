---
id: 239
title: Overlays draw clean - no hex cursor halo, no black box on the edge cursor, the shot mark erased
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Two cursor faults the owner saw on 2026-11-08 (kladwtwn, windowed, scale 2,
3440x1440, sharp text on):

1. The red hex move cursor has dotted ghost outlines around it: one or two
   larger dashed rings of the same red, as if older or scaled copies of the
   cursor stay drawn. Crop: `runs/239-cursor-artifacts/hex-cursor-halo.jpg`
   (his shot `20261108-181235-kladwtwn-e0`).
2. At the map's top edge the scroll cursor (arrow with the red X, "cannot
   scroll") sits in an opaque black box: its transparent pixels draw black.
   Crop: `runs/239-cursor-artifacts/edge-cursor-black-box.jpg` (his shot
   `20261108-181209-kladwtwn-e0`).
3. Stacked here (the owner, 2026-11-08): the screenshot region mark
   (Ctrl+F12, task 183 step 2, 218) stays drawn on the bottom interface
   panel after the mark ends and is never erased. Same family as 1: pixels
   drawn over the interface or the 3D view that nothing clears.

Find why each happens, fix it at the root, and show before and after shots
from the test instance.

## Acceptance

- The hex cursor shows only its own outline, while moving and when still.
- The edge scroll cursors keep their transparency over the 3D view.
- After a region mark is saved or cancelled, the bottom panel shows no
  trace of it.

## Log

- 2026-11-08 claude: cut from the owner's shots.
- 2026-11-08 claude: stacked the region-mark find from the inbox (part 3); suggested glm.
- 2026-11-08 claude: owner order 2026-11-08: glm, first in its queue: 239, 172, 211, 212, then 224.
- 2026-11-09 glm: merged as #371 (`0a825d85a`), CI green; the F12 path item from the inbox rode along.

### 2026-11-08 glm
Done: the repro tooling and the first attribution. The scripted-input
language gained `gmove <x> <y>` (preferences_probe.cc): the game's own
cursor moves through _mouse_set_position plus the game's hover event
(_gmouse_handle_event), because the SDL-level `move` never reaches the
game's cursor state. The hex outline (an object) renders in devshots
(runs/239-cursor-artifacts/repro2-hex-clean.jpg), clean, no halo on a
static park. Scroll zones need the exact edge pixel (y = 0, not 1+).
Attribution leads: (1) _mouse_blit_trans is null in this codebase
(input.cc:407, svga.cc:216), so mouseShowCursor composites the saved
under-buffer with the shape and plain-blits the whole rect per move,
and mouseHideCursor restores - a save/restore composite over a
background that changes under it (the 3D view glides) and at scale 2;
this is the halo's prime suspect. (2) The mouse sprite (arrow, scroll
cursors) never reaches the presented frame in a background devshot:
repro3/repro4 park the cursor on a walkable tile and on the exact top
edge (runs/239-cursor-artifacts/repro4-edge-nosprite.jpg), the game
logic follows (the hex follows the gmove, the view scrolls at y=0),
the sprite does not appear. The sprite's absence blocks the test
instance's before/after for the black-box fault.
Next: find where the sprite blit drops in the devshot's present path
(mouse.cc mouseShowCursor's blit vs the overlay's shown mask), then
reproduce the halo with a moving cursor over the animated scene, and
the black box once sprites render. Both owner crops live on main
(runs/239-cursor-artifacts/).
Check: tools/dev-snapshot.ps1 -Map kladwtwn -SceneInput
runs/239-cursor-artifacts/repro4/input.txt -DevInputShots 5
-OutputDirectory runs/239-cursor-artifacts/repro4 -CaptureConsole

### 2026-11-09 glm
Done: all three faults fixed at the root and proven on the live game window
(new runs/239-cursor-artifacts/run-live.ps1: runs the game with a real
visible-or-offscreen window, pumps the scripted input, captures the window
with PrintWindow; the old background-devshot path cannot show the mouse
sprite at all - it re-draws the last present's texture, and no real SDL
mouse exists there).

1. Halo: the hex cursor is drawn twice per frame. The outline pass
   (objectVisitOutlined) projects the flat 32x16 hexagon through the 3D
   camera (objectDrawSceneOutline), so once the view turns or zooms a
   scaled dotted ring lands next to the true ring the cursor pass draws.
   Fix: scene_overlay.cc drawOutline skips gGameMouseHexCursor and
   gGameMouseBouncingCursor - their outline belongs to the cursor pass.
2. Black box: the sharp-text cache's cursor patch (patchCyclingRegion,
   text_present.cc) indexed its mask loop from column 0 instead of the
   box's left column: it masked a strip at the screen's left edge and left
   the box's own see-through pixels opaque palette black. Fix: the row
   pointer starts at x0. Proven by the box vanishing with
   --dev-sharp-text=0 or F2CE_NO_SHARP_CACHE=1; the earlier
   _mouse_blit_trans transparent-blit experiment was reverted from the
   tree (the composite was not the culprit).
3. Stale mark: over the 3D view the rectangles already ride the RGBA
   overlay; the raw indexed painting only ever reached pixels nothing
   refreshes. Fix: owner_shot.cc draw() takes the overlay path in the 3D
   full view, and exitMarkMode() re-blits the screen once
   (windowRefreshAll) so the last mark leaves no trace.
4. Stacked inbox item: F9/F12 shots move from shots/dev/<date>/ to
   <game>/screenshots/ (owner_shot.cc, PNG + JSON together), and the
   message line names the path: "скриншот сохранён: screenshots\..." (a
   test run's F2CE_DEV_OUT folder shows absolute). The dev snapshots keep
   shots/dev/<date>/; docs/owner-shots.md updated.

Evidence (JPEG, committed): 239-halo-before-after.jpg (dotted ghost ring
gone after zoom), 239-scroll-box-before-after.jpg (transparent scroll
cursor at the exact top edge), 239-mark-before-after.jpg (no green line on
the bottom panel after Enter), 239-screenshots-path.jpg (the message line
names screenshots\<file>.png). Raw frame folders stay untracked in live/.
The scripted mouse ("move") drives the OS cursor: a live window reads the
real mouse each frame, "gmove" answers only in background devshots.
Check: runs/239-cursor-artifacts/run-live.ps1 -Background -InputFile none
-Extra --dev-owner-shot-once=8 (shots land in <run>/screenshots/ under
F2CE_DEV_OUT, in <game>/screenshots without it); the A/B pairs above come
from runs/239-cursor-artifacts/live/{before,after}-frames and mark-*.
