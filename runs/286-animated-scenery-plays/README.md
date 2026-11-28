# 286: animated scenery - the inventory and the capture-method verdict

## Deliverable 1: the inventory (from the game data, no captures)

The palette's animated ranges (f2lib/pal.py, cycle.cc): slime 229-232,
monitors 233-237, fire_slow 238-242, fire_fast 243-247, shoreline 248-253,
alarm 254. Scanned every scenery object and floor/roof tile of seven
representative maps (kladwtwn, modmain, sfdock, klatoxcv, vault13, newr1,
sfchina) for frame counts over one and pixels on those ranges.

Palette-animated kinds and their representative maps:

| kind | maps | art |
|---|---|---|
| fire (slow+fast) | kladwtwn t15097/15101/21727/28701, modmain, sfdock, newr1 | barrel.frm (10+79 px), woodfire.frm, atorch3/4/5 (Modoc torches) |
| slime | klatoxcv (goop01-04 on 52 objects), sfdock (goop01/03/04, barrel7) | goop01 = 292 px |
| monitors | vault13 t13675 (monitr7 = 316 px, comp6), newr1 t15062 (monitr8), sfdock monitr3 | monitr3-8, comp1-6 |
| shoreline | sfdock (seas001-018 tiles, shorcar2/shorjnk2 props) | seas tiles carry 3-56 px each |
| alarm (1 px) | vault13, newr1 | comp3/comp4 |

Frame-animated ambient kinds (frame_count > 1): FLIES01/02 (10 frames),
SMOKE01 (12, fps 8), steam01 (12, fps 8), POWER001 (15), v13secur/v13secr2
(15), TANKERW1 (6), curtain (8, fps 10), MODWELL1 (3). The door FRMs
(wdoor, doorb, necdoor, casent, cathdor, gate, velev05, v13doorf/s) carry
open/close frames - the interactive class (284), not ambient animation.

The 3D already carries the palette mechanism: the exporter marks animated
texels (tiles alpha 252, sprites 254, task 022) and the scene shader
substitutes the cycling colours. Whether every kind visibly cycles in the
live 3D view is exactly what the pairs below could not yet prove.

## Deliverable 2: the capture-method verdict - METHOD NOT CONFIRMED

Four protocols tried to record movement inside the object's zone; none can
see animation in a background run today:

1. still + -HoldSeconds: identical frames. The hold runs after the snapshot,
   and the still fixture freezes the world at a fixed tick anyway.
2. owner-shot delays 1 s / 4 s (live capture mode): identical frames. The
   sidecar's game_time sits at 302401/302402 in every run: the world clock
   does not advance in the static-preview fixture.
3. -NoPreview: the startup still labels the run static-wall-plus-hero
   preview and the clock stays pinned.
4. the 239 live-window harness (run-live.ps1, PrintWindow every 150 ms):
   the window runs, but offscreen the world update starves - the classic's
   game clock advanced 1 tick in 14 s; the cursor blinker moves (1-2k px)
   while the slime pools measured 0 across consecutive frame pairs; one
   late 7000 px cluster shows the cycle does reach the pools eventually.
   A Ctrl+F10 chord was delivered (the input steps logged) but the frames
   stayed in the original view.

Per the review criterion: the classic side itself does not demonstrate
movement reliably in these captures, so nothing can be said about the 3D
yet - neither "moves" nor "static". Evidence:
`01-slime-classic-frozen.jpg`, `02-slime-3d-frozen.jpg` (delay 1 s vs 5 s,
pixel identical), and the raw frames in build/walk-286/.

## What the pairs need (named, not built)

A capture from a live world: the 239 harness WITHOUT -Background - a
visible focused window - or a small fixture that lets the world update in
an offscreen window. The focused-run freeze in _GNW95_lost_focus is real
(run-live's own header documents it), so the visible run must keep focus
for its whole length. The method then is: one run per engine, ~12 s,
frames every 150 ms, diffs inside the pool's bbox - exactly protocol 4.

Until then the kinds stay unproven in both directions; the inventory above
is the deliverable this round adds.

## 2026-11-27, the live-window round: THIS pool moves in both engines

The capture that works: the 239 live-window harness WITHOUT -Background (a
visible focused window over the separate stock install - build/stock-root:
master.dat, critter.dat, sfall.dat, mods/, data/MAPS/AUTOMAP.DB and the fixed
test cfg; no owner game, saves or settings touched). PrintWindow of the
presented framebuffer every 150 ms nominal; the actual gaps settled at
245-278 ms (classic) and 249-305 ms (3D). Klatoxcv e1, view tile 22924, hero
19291 (card 073's pool).

Transition frames excluded: the --dev-new-game start opens through the
Temple of Trials map before the dev target loads, so frames 0-4 (classic)
and 0-3 (3D) are the loader and another map; the stable ranges below start
after the caves are on screen.

- Classic positive control (frames 5..16, eleven pairs at 245-278 ms):
  the pool zone changes by a steady 2300 px per pair, eleven of eleven,
  while the neighbouring still-rock control zone is 0 on every pair.
- 3D positive result (frames 7..14 after the scripted Ctrl+F10 chord at
  1.5 s; 249-305 ms): the pool zone changes by 779/968/669 px on the
  first three pairs with the control zone at 0; the later pairs read
  42/2/1/45 px - the sparse samples caught same-phase moments of the
  discrete cycle.

Verdict, scoped to THIS pool (klatoxcv e1, the goop pool by the barrel at
view 22924): it animates in both engines; no bug. No pace or cadence claim
is made - the classic steps every pair while the 3D samples straddle the
cycle unevenly, and a pace comparison needs a controlled-interval pass.
Nothing here speaks for other water, other maps or the other kinds; the
inventory above remains the map of what to shoot next (fire, shoreline,
monitors, alarm, the frame-animated ambient list).

Sheets rebuilt from the stable frames only:
`03-slime-classic-live-water-strip.jpg` (classic frames 5/7/9/11, the pool
crop, the +px deltas named), `04-slime-3d-live-water-strip.jpg` (3D frames
7/8/9/10, the pool crop at 1.5x). Raw frames and their mtimes stay in
build/walk-286/vis-slime-cl and vis-slime-3d.

## 2026-11-27, the fire round: classic confirmed, the 3D needs the scene camera

Fire, kladwtwn woodfire (tiles 22958/22956, the live visible window):
- Classic positive: the flame flicker clusters read 1300-5400 px per pair
  across frames 10-17 and 35-41 of the raw run (the flame + the camp
  critters together; the focus bursts freeze the world for a few pairs -
  the visible window pauses on lost focus, documented in run-live's own
  header - so the run reads as bursts).
- 3D: the Ctrl+F10 chord flipped the live game into the scene view (the
  flip seen at frame 8-9), and the scene view is LIVE - the three camp
  critters' idle animations blink at 400-500 px per pair in the frame.
  But the scene view's camera is spawn-centred (it framed the brahmin pen
  at kladwtwn's spawn 25075), so the woodfire sits outside the frame and
  the fire ROI was not captured. The scene camera does not follow
  --dev-view-tile/--dev-hero-tile in the live toggle view.

The spawn-distance audit (hex distance from each map's spawn to the
nearest kind object, the scene view frames the spawn area):

| map | spawn | nearest fire | nearest slime | nearest monitors |
|---|---|---|---|---|
| modmain | 20100 | atorch3 @20505, d=5 | - | - |
| newr1 | 24305 | barrel @23512, d=7 | - | monitr8 @15062, d=46 |
| klatoxcv | 20100 | - | goop02 @21507, d=7 (verified) | - |
| sfdock | 20100 | barrel @22284, d=16 | goop01 @18093, d=10 | monitr3 @16251, d=49 |
| vault13 | 17095 | - | - | monitr7 @13675, d=20 |
| kladwtwn | 25075 | barrel @28701, d=26 | - | - |

Verifiable now with the scene view (the kind in the spawn frame): modmain
fire, newr1 fire, klatoxcv slime (done), probably sfdock slime. Need the
scene camera control first: kladwtwn fire, vault13/newr1/sfdock monitors,
the sfdock shoreline.

### The offscreen reproduction (a method note, not a substitute)

The same slime pair re-run with `-Background` (the fully offscreen window,
no focus pauses at all): the pool zone cycles at 28-72 px per 150 ms pair
with the control zone at exactly 0 - the offscreen world does tick, just
at a slower cadence than the visible window. Per the review criterion the
presented-frame numbers above remain the ones that count; the offscreen
reproduction is recorded as the method's fallback and as the explanation
of the earlier frozen runs (the visible runs froze on lost focus during
the double map load, which the -Background mode never hits).

The modmain fire pair (classic+3D, visible) froze the same way mid-run
(thirteen zero-diff pairs after the double load), so its frames stay out
of the verdict; the fire kind waits for either a focus-holding visible
run or the owner's word to accept offscreen evidence.
