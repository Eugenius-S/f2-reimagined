# 329: the CLOSED ART find does not reproduce

Verdict: no defect. The 281 sheet compared two different tiles; at the tile the
cards actually live on, both engines draw the same placeholder art. No code
change ships with this folder.

## What the 281 evidence really paired

The walk's raw captures in `build/walk/281/` carry their tiles in the devshot
sidecars:

| capture | engine | view tile | what it shows |
|---|---|---|---|
| devshot-20261125-224841 | classic 2D | 20985 | empty black area |
| devshot-20261125-224237 | 3D game camera | 20695 | the red card field |

Tile 20985 (the sheet's classic half) is an empty part of the map. Tile 20695
(the sheet's 3D half) is the shuttle pad, where 94 floor squares carry tile
id 2479 = `shut058.frm`, a 42x18 red card whose picture reads UNUSED ART
(`04-shut058-frm.png`). The 3D lays floor art flat like every floor, so the
squashed text read as "CLOSED ART!" in the small sheet.

## Fresh captures, both engines, each tile (2026-11-26)

Build: `E:/Projects/f2r-279-glm` worktree, `build/fallout2-ce.exe`
sha256 d6ad165260a22c7e, the walk's own build; scene cache
`build/scene-cache/sfshutl2` (exported 2026-11-25 22:33, exporter 1.13.0),
also the walk's own.

- `01-pad-tile-20695-both-engines.jpg`: tile 20695. Classic: upright UNUSED
  ART cards, as it draws every floor tile. 3D: the same field flat. Same art,
  same squares, hero at the same spot in both.
- `02-tile-20985-both-empty.jpg`: tile 20985. Empty black in both engines.
- `03-cowbomb-missing-art-both-empty.jpg`: COWBOMB e0 tile 21165, control for
  the one-sided rule: 25 scenery objects whose `BLACK001.frm` is missing from
  the data draw nothing in the classic, and nothing in 3D either.

Commands (one per side, `-Classic` picks the classic engine):

```
tools/dev-snapshot.ps1 -Map sfshutl2 -Elevation 0 -ViewTile 20695 -Still -Classic -OutputDirectory <out>
tools/dev-snapshot.ps1 -Map sfshutl2 -Elevation 0 -ViewTile 20695 -Still -SceneCache build/scene-cache -OutputDirectory <out>
```

## Placeholder sweep over the whole game

`build/scratch-329/scan_missing_art.py` (run folder keeps the numbers, the
script stays scratch) walked all 176 maps in the game tree and resolved every
floor and roof tile id plus every object FID through the same source chain the
engine uses:

- 23250 floor and 6166 roof squares: no tile art missing anywhere, and no
  tile whose art fails to load - the 2D never draws nothing for a tile, so
  the one-sided placeholder rule has no tile case to cover.
- Objects: five art misses in the whole game - `cowbomb` e0, 25 scenery
  objects on `BLACK001.frm` (missing from the data, nothing draws in either
  engine, see 03); critter art variants `hmmaxx` (epax e1), `hanpwr`
  (mbase34 e0), `nmmaxx` (rndparih e0), and one out-of-range critter FID on
  vault15 e2 - a critter whose art fails to lock draws nothing in the classic
  and its live 3D card fails on the same FRM, so nothing draws there either.

The red UNUSED ART field is art the 2D draws, so per the task's own split it
stays visible in 3D (283 already recorded the match in
`runs/283-roofs-open-over-the-hero/README.md`). Closing 329 as done without a
renderer, exporter or cache change.
