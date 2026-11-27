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

## 2026-11-27, the live-window round: the slime kind moves in BOTH engines

The capture that works: the 239 live-window harness WITHOUT -Background (a
visible focused window over the separate stock install: master.dat,
critter.dat, sfall.dat, mods/, data/MAPS/AUTOMAP.DB and the fixed test cfg
in build/stock-root; no owner game, saves or settings touched). Frames every
150 ms via PrintWindow of the presented framebuffer, 12 s per engine.
Klatoxcv e1, view tile 22924, hero 19291 (card 073's pool).

- Classic positive control: from frame 3 on, the pool zone changes by a
  steady 2300 px per frame pair while the neighbouring still-rock control
  zone changes by exactly 0. The slime cycles.
- 3D: the same run with a scripted Ctrl+F10 chord at 1.5 s (the input
  pump's hold/key/release, waitms-based) flips the live game into the 3D
  view; the pool zone then changes by 779/968/669 px per pair (frames 4-7)
  with the control zone at 0. The pool quiets later (42/2/1 px) - the
  capture spacing jitters around the cycle's step, so the cadence detail
  needs a controlled-interval pass, but movement itself is proven.

Verdict for the water/slime kind: moves in both engines, no bug. The
earlier frozen/offscreen numbers in this folder were fixture artefacts, not
evidence either way. Sheets: `03-slime-classic-live-water-strip.jpg`,
`04-slime-3d-live-water-strip.jpg` (the pool crop, 1.5x). Raw frames stay
in build/walk-286/vis-slime-cl and vis-slime-3d.

The other kinds (fire, monitors, shoreline, alarm, the frame-animated
ambient list) still need this same live protocol per kind; the inventory
above stands as the map of what to shoot.
