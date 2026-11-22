# 275 - encounter maps' upper floors are not black in 3D

## The mechanism, traced first (2026-11-19)

Fresh classic captures (sidecar-verified DENBUS2.MAP / DESERT5.MAP,
elevation 1, vanilla-framebuffer): the upper elevation's own data is a
handful of floor squares at the map's north edge (desert5 e1: 19 squares,
tile 659 bld2043.frm; desert6 e1: 7; denbus2 e1: 83), 0 objects, 0 roofs -
and the classic 2D view shows the LOWER elevation's scene around it.

The band's engine path: vanilla's damage-rect persistence.
`tileWindowRefreshRect(&rect, obj->elevation)` (actions.cc 541/2202,
animation.cc throughout) repaints damage rects at the OBJECT's OWN
elevation - animated lower-scene objects (the tents' fires, walking
critters) accumulate lower-scene rects in the persistent 2D framebuffer
even while the player is on the upper floor. It is an artifact of the 2D
damage system: the band's extent is whatever happened to animate. There is
no deliberate band render to reproduce; every per-elevation render pass
draws the current elevation only.

## The fix (this PR)

The card's own prescription - 173's plane fallback, never black - took the
form that survives the raw-buffer proof: a nearly-empty upper floor (fewer
than 100 drawn squares, `staticFloorDrawnSquares()` in the renderer) shows
THE ELEVATION BELOW's scene instead of a black void. Its cache exists (the
export builds every elevation), the hex grid is shared so the camera lands
on the same spot, and the live hero draws over it at his own hex. The
state machine stays consistent: the game's elevation is kept in
`gLoadedElevation`, and walking down to the lower elevation reloads its
own scene normally. Trigger and log line:
"scene: desert5 elevation 1 has 19 drawn squares; showing the scene of
elevation 0 below".

Supporting changes: `staticFloorDrawnSquares()` (floor.cc/static_scene.h);
`mapRenderWholeIsoWindowAt(elevation)` (map.cc) - the 273 wait plane's
repaint parameterized by elevation (the tile renderers take their per-hex
light from gElevation, so the paint swaps it).

## Evidence

- desert5-e1-classic-vs-3d.jpg: classic 2D (left, the tents and the desert
  through the damage-rect artifact) vs 3D (right: the elevation below's
  scene - the desert floor under the hero, the strip's own tiles at the
  edge).
- desert6-e1-3d.jpg: the same rule on desert6 e1 (7 own squares).

## Honest limits

- The brightness: the classic band reads brighter (its rects were painted
  by the lower scene's own light sources near the fires); the below-scene
  render carries the desert's own baked light (a dim night encounter), so
  the 3D frame is darker than the 2D artifact. The card's brightness
  criterion is not met yet; the world is visible and never black.
- The plane-texture detour (the first implementation showed the 2D art on
  the ground plane) was abandoned after the raw-buffer dump proved the
  repainted buffer decodes dark through every palette source - the dump
  (960x440 indices + the 768-byte palette) is preserved in this session's
  notes; the below-scene design replaced it.
- The class scan (every elevation over 20% black in 271's table) needs the
  full sweep rerun after this lands; the two named maps plus the trigger
  rule cover the class by construction.

## Checks

- ctest 29/29 (the trigger, the state machine, the renderer changes).
- The scene_export suite: 152/152 (untouched by this PR, run as a guard).
- Live runs: desert5 e1 and desert6 e1, the fallback triggered and the
  log lines captured in this session's notes.
