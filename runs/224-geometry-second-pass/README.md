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

- **F1** kladwtwn e0 — top-right map edge renders as a black wedge
  in 3D; 2D shows the dirt road continuing to the edge there
  (view tile 24678). Class: black/cut map edge.
  finds/f1-kladwtwn-e0-3d.jpg vs finds/f1-kladwtwn-e0-2d.png
- **F2** denbus2 e1 — 3D scene has only a small lit dirt strip +
  corner patch; 2D at the same elevation renders the whole level
  (diner, sidewalk, street, ruined shack). Whole-elevation geometry
  missing from the scene/export. Class: missing pieces.
  finds/f2-denbus2-e1-3d.png vs finds/f2-denbus2-e1-2d.png
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
  grey rocks at the right edge, 3D looks bare there) — needs a zoomed
  probe before claiming.

## Harness

- Sweep driver: build/sweep-driver-gaps.sh (bash) + a validator
  build/validate-sweep.py (sidecar JSON vs expected config).
- Lesson: backgrounded driver launches doubled themselves twice and
  raced the test-game slot (transient FAILEDs, duplicate shots);
  a single detached PowerShell Start-Process launch is the reliable
  pattern.
- The five hanging maps: tasks/246-devshot-hang-club.md.
