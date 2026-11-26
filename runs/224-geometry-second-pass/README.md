# 224 — geometry second pass: city sweep (agent-side precursor of 175–182)

## What ran

2026-11-10, exact main build ba79b58d (#395 in). For every city
map-elevation in the eight cities of the owner's review order
(Arroyo, Klamath, Den, Modoc, Vault City, Gecko, Broken Hills,
New Reno): a 3D devshot at hour 12, one at hour 18 (both
`-Tour "<map>:<elev>"`), and a classic 2D frame of the same camera.

- 66 planned groups × 3 runs = 198 frames.
- 183 frames valid and judged; the full shot set stays on the
  machine (runs/224-geometry-second-pass/sweep/, ~420 MB — not
  committed; every frame's sidecar JSON was machine-validated
  against its expected map/elevation/hour/view).
- 15 frames could not be taken: five maps hang the engine at load
  (classic mode included) — own task 246.

## Finds (one line per find, per the task's acceptance)

- **F1** kladwtwn e0, top-right map edge: FIXED on main (202's
  quadrant fill, #320). Fresh frame f1-kladwtwn-e0-now.jpg: the road
  continues to the edge, no wedge (2026-11-12).
- **F2** denbus2 e1: NOT a 3D defect (2026-11-12). The map's own e1
  data: 9917/10000 floor squares are the void tile, 66 drawn (the dirt
  strip), 0 objects. The fresh classic 2D frame renders the same
  content the 3D does; the old find's description over-read it. The
  real residual: the vanilla 2D draws the tall walls of elevation 0
  that reach into the elevation-1 view, the 3D upper scene does not -
  the next exporter rule. Pair: f2-denbus2-e1-2d-vs-3d-now.jpg,
  fresh frames f2-denbus2-e1-{2d,3d}-now.jpg.
- **F3** newr2 e0 — the Shark Club marquee: 2D shows the full lit
  «SHARK CLUB» sign; 3D erases its upper part into a black band
  (letters cut, bulbs row intact). Class: roof/wall hole.
  finds/f3-sharkclub-e0-3d.jpg vs finds/f3-sharkclub-e0-2d.png

All three are exporter/renderer-level, not per-map data nits —
each needs its own diagnosis (F1 map-edge fill, F2 floor extraction,
F3 marquee card/cap); none is a small in-224 fix.

## Non-finds worth remembering

- arcaves e2 / klatoxcv e2 / broken1 e1 / klamall e1 / newr1 e1:
  mostly-dark frames are 2D-faithful (camera over void or unlit
  interiors) — verified against classic pairs.
- arcaves e0 "green floor" candidate: renders in 3D too (seen in
  the arcaves e1 pair) — ambient difference, dropped.
- newrcs e0: the black ellipse by the Pep Gas sign is baked 2D art,
  reproduced faithfully.
- vctyvlt e2: the spiky black blob behind the hero is in 2D too —
  vanilla art.
- Low-confidence leftover: artemple e0 right-edge boulders (2D shows
  grey rocks at the right edge, 3D looks bare there): probed 2026-11-27,
  see "The right-edge boulders pair" below: the top grey pile renders in
  3D; the pair's cameras do not pixel-align, so the claim stays closed
  as inconclusive rather than a find.

### The right-edge boulders pair (2026-11-27)

One narrow control pair from a separate stock install (no owner game or
saves): artemple e0, view tile 17288, hour 12, 1920x1080 both sides,
the same centre-tile intent. Files here: `boulders-pair-classic-edge.png`
/ `boulders-pair-3d-edge.png` (the x 1600-1920 strip) and the full frames
as JPG. Exact frames and SHA-256:

- classic `scr00000.png` sha256 e90522aa4cb362dcb8b3effe2ef3c9a1183d55656686715fa4cd4d8d11fb6318
- 3D `devhero-20261127-060018-000.png` sha256 01680412191bc91fa77c3be3a5a885f41cfd5dd82dbfc27e27c096fd4af6ee0f

Findings:

- The grey rock pile at the strip's top (roks3.frm on tiles 17103, 17104,
  17106 and 17304) renders in BOTH engines - the 3D shows it as shaded
  cards. The original "3D looks bare" claim does not reproduce there.
- The classic's bottom-right grey rock (the roks/roks2/rock07/rock10
  objects on tiles 17513..18915) has no clear in-frame 3D counterpart:
  the 3D static preview's camera is wider and shifted versus the
  classic's centre-tile camera, so the same screen coordinates do not
  name the same world points.
- Verdict: the pair's cameras do not align, so per the review criterion
  this stays inconclusive - no rule change, no mask change, no find
  registered. The data linkage stands on its own: the rocks at the
  entrance's right edge are roks3.frm (6 objects) plus
  roks/roks2/rock07/rock10 (5 objects), plain scenery the export does
  carry.

## Harness

- Sweep driver: build/sweep-driver-gaps.sh (bash) + a validator
  build/validate-sweep.py (sidecar JSON vs expected config).
- Lesson: backgrounded driver launches doubled themselves twice and
  raced the test-game slot (transient FAILEDs, duplicate shots);
  a single detached PowerShell Start-Process launch is the reliable
  pattern.
- The five hanging maps: tasks/246-devshot-hang-club.md.
