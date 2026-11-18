# 276 - chain-link fences show what is behind them

## Root cause

The Den's chain-link fences (denbus1; fence01/04/15/18, art indices 334,
337, 348, 351) stood as solid black walls with bright specks. The wire
draws fine; the MESH HOLES were black. Cause: task 115's anti-sparkle fill
(`props.fill_holes`) runs on every wall face and fills the art's enclosed
transparent texels with the edge colours - the fence's mesh holes are
exactly such texels, so the see-through holes turned into a black plane.

## The fix

`vendor/fp/scene.py mesh_face()`: a face whose picture is mostly
see-through (the fence FRMs measure 42-58% opaque; a solid wall's picture
is 90%+ opaque) is a mesh and keeps its holes - the alpha cut-out shows
what is behind, and the wire keeps its colour. Solid walls fill exactly as
115 built it. `tools/scene_export/test_mesh_face.py` pins the two classes
and the threshold between them.

## Maps that use this fence art (by art id)

fence01 (334), fence04 (337), fence15 (348), fence18 (351): 30 maps -
abbasem, abbey, denbus1, denbus2, denres1, depolv1, geckjunk, klamall,
mbclose, modinn, navarro, ncr1, ncrent, newr1, newr2, newr3, newrba,
newrcs, newrst, raiders1, raiders2, redment, redwame, sfchina, sfsheng,
sftanker, slvcamp, vctycocl, vctyctyd, vctydwtn. Every one changes with
the rule (their scenes rebuild).

## Evidence

denbus1 at the fences (hex 14532), before (left) | after (right):
- denbus1-fence-game-camera.jpg: the game camera. Before: black walls with
  specks. After: the mesh is see-through - the ground, the truck and the
  concrete pad show through the holes; the wire keeps its colour.
- denbus1-fence-turned-view.jpg: one turned view, the same verdict; the
  diamond mesh reads as a mesh.

The captures: the same build, the same camera (`-ViewTile 14532`, and
`-Turn 2` for the turned pair); the only difference is the scene cache:
before = the exporter without the gate (the 273 worktree's unmodified
scene.py), after = this branch's.

## Checks

- tools/scene_export/test_mesh_face.py: 4 tests.
- scene_export suite: 152/152.
- The fence arts' faces keep their holes: the denbus1 atlas pages measure
  53.9%/55.8% transparent texels (the holes; before the fix they filled).
