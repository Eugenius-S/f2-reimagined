---
id: 275
title: Encounter maps' upper floors are not black in 3D
milestone: M1
status: doing
owner: glm
suggested: glm
priority: normal
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The upper elevations of the desert and coast encounter maps show in 3D what the original shows, not a black screen.

## Context

- Folded into 173 on 2026-11-05 and cut out here: desert5 e1 and desert6 e1 export with no floor art and no sprites (`walls.bin` 24 bytes, empty `tiles.png`, `indoor=true`), the 2D mean brightness is 46 against 0.8 in 3D. Shots: `runs/169-encdesertcoast-in-3d/finds/`.
- 271's table lists every elevation over 20% black: fix the whole class, not two maps.

## Acceptance

- Classic | 3D pairs for desert5 e1, desert6 e1 and every other elevation of that class in `runs/275-encounter-upper-floors-not-black/`; the 3D mean brightness within a third of the 2D one.
- An elevation the exporter still cannot build stands on the plane fallback (173's rule), never black.
- Exporter tests, ctest and the pixel regression pass.

## Out of scope

Cave lighting levels (262, 263).

## Log

- 2026-11-13 claude: cut for glm's weekend queue (the owner: more tasks for glm).
- 2026-11-19 glm: the mechanism traced first (fresh classic captures, DENBUS2/
  DESERT5.MAP elevation 1 verified in the sidecars): the 2D upper-floor view
  shows the LOWER elevation's scene through vanilla's damage-rect
  persistence - tileWindowRefreshRect repaints at the OBJECT's own elevation
  (actions.cc 541/2202, animation.cc throughout), so animated lower-scene
  objects accumulate lower-scene rects in the persistent 2D framebuffer even
  while the player is on the upper floor. It is an artifact of the 2D
  damage system, not a deliberate render rule; the band's extent is whatever
  happened to animate. The upper floor's own data: desert5 e1 = 19 floor
  squares (tile 659 bld2043.frm at the map's north edge), desert6 e1 = 7,
  0 objects - the exporter builds them fine (the atlas carries the art; the
  424-byte tiles.png is a valid palette PNG).
- 2026-11-19 glm: the fix per the card's own prescription - the 173 plane
  fallback. An upper elevation whose floor is nearly empty (< 100 drawn
  squares, `staticFloorDrawnSquares()`) shows the elevation below's own 2D
  render on the ground plane instead of a black void: the 273 wait-plane
  machinery, repainting the iso window at the LOWER elevation
  (`mapRenderWholeIsoWindowAt(elevation)`, the tile renderers take their
  per-hex light from gElevation, so the paint swaps it). Trigger, view
  state, log line all work: "scene: desert5 elevation 1 has 19 drawn
  squares; showing the elevation 0 on the ground plane". ctest 29/29.
- 2026-11-19 glm: OPEN DEFECT, honestly named - the plane renders DARK
  (its texture decodes to ~6/255 brightness through the palette). Verified
  by elimination: not the shader state (uTextured/uGroundBakeMode/light
  mode checked and set), not the screen-surface palette (the engine's own
  palette decodes dark too), not the light ambient (both extremes probed:
  the ambient semantic is INVERTED - LIGHT_INTENSITY_MAX is the darkest,
  MIN the brightest - and neither brightens), not the gElevation swap (the
  paint's mean index stays 110.5 with and without). The texture = the iso
  buffer through the palette, and that combination reads dark even though
  the classic 2D frame of the same data reads bright (46). The next session
  starts here: dump the repainted buffer to a file (the in-game lodepng
  write failed with error 84 even on an absolute path - write it from the
  debug build differently) and compare against the classic frame's pixels.
  The PR stays a draft; no acceptance is claimed.
- 2026-11-19 glm, later: the raw dump (960x440 indices + the 768-byte
  palette, saved via plain ofstream - the lodepng path failed) settled the
  plane detour: the repainted buffer decodes to a dark wedge (the darkened
  strip/void), NOT the classic's bright band - the 2D band comes from the
  damage-rect accumulation (see above), which a fresh repaint cannot
  reproduce. The plane approach is abandoned. The fix now loads the
  ELEVATION BELOW's scene from its cache (the hexes are shared, the camera
  lands on the same spot, the hero draws over it): desert5 e1 and desert6
  e1 both show the world below - never black (the Log's defect above is
  closed by the design change; the brightness still trails the 2D
  artifact's, honestly: the below-scene carries the desert's own dim baked
  light, the artifact carried the fires' local light). Evidence:
  runs/275-encounter-upper-floors-not-black/. ctest 29/29.
