# 274 - upper floors show the lower elevation's tall walls

## The find, re-established (2026-11-19)

denbus2 e1's own map data is 83 floor squares (a dirt strip at the map's
north edge, rows 0-2), 0 objects, 0 roofs. The classic 2D view on e1
nevertheless shows the street and the building: the game draws the LOWER
elevation's scene around/behind the upper floor's patch (the frames below).
The 3D upper scene had none of it.

Fresh classic captures: `dev-snapshot -Map denbus2 -Elevation 1 -Classic`
(sidecar-verified DENBUS2.MAP, elevation 1, vanilla-framebuffer).

## The rule

`vendor/fp/scene.py`: an upper elevation's scene carries the lower
elevations' wall objects (not hidden), drawn first like the lower scene is.
VISUAL ONLY: their faces stay non-solid (faces_of) and out of this floor's
blocking (build_blocking), so the upper floor's walkability and light grid
keep the original's. `export.py`: the sidecars take the builder's combined
object list, so the rows' serial numbers name every wall. The exporter
version is 1.14.0.

The renderer side (`walls.cc`, `scene_view.cc`): the loaded walls' ground
rectangle unions into the map clip when the clip comes from the floor hull,
so a tight floor clip never hides a wall the scene carries.

## Evidence

Classic (left) | 3D (right) pairs, same -ViewTile camera, denbus2 e1 at
hex 16497, denres1 e1 at 12657, ncrent e1 at 19314:
- denbus2-e1-classic-vs-3d.jpg: the lower walls over the void, the case the
  card names.
- denres1-e1-classic-vs-3d.jpg: a full upper floor over lower walls.
- ncrent-e1-classic-vs-3d.jpg: a dark interior (its own ambient), walls render.

## Byte-for-byte

Maps without lower walls export unchanged: denbus2 e0's game-read files
(floor/walls/baked/shadows/projection.bin, every png) are byte-identical
across the rule; scene.json differs only in the exporterVersion stamp.
Check: sha256 per file, build/e0-after vs build/e0-before (this machine).

## Known limits (honest)

- The 3D shows the lower walls everywhere they stand; the 2D's lower scene
  appears in the band the map-edge system draws. The exact engine path of
  that band was not traced to its code (every per-elevation render pass I
  read draws the current elevation only, yet the 2D frame shows the lower
  content); a zone-limited rule is a possible refinement if review wants
  the 3D band narrower.
- The lower walls keep the lower elevation's light in the 2D composite; the
  3D lights them with the upper floor's ambient (their faces take the scene's
  light at their hexes). Close, not pixel-equal.
- The lower floors' tiles are out of scope (the card): the 3D walls stand on
  void where the 2D shows the street beneath them.

## Checks

- tools/scene_export/test_lower_walls.py: 3 tests (e1 carries the lower
  walls, the ground floor stays alone, the serials cover everything).
- scene_export suite: 151/151.
- ctest: 29/29 (the renderer's clip union included).
