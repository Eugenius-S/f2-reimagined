# Save preview in the load dialog — verification on merged main

The fix itself is #297 (task 134's recovery, merged 2026-11-07). This run
verifies it still holds on the current build (ce_commit ebad0465a, which
contains 7e01a4d61), per task 212.

Two full save→load round trips on the demo Klamath startup, 1920x1080,
scale 2, isolated test roots:

- `3d/` — save made with the 3D scene active:
  `SAVE_PREVIEW_FIXTURE PASS map=KLADWTWN.MAP mode=3d save_ui=1 load_ui=1 saved=1 loaded=1`
  `3d/scr00001.png` — the load dialog («ЗАГРУЗИТЬ») draws the saved slot's
  picture: the 3D-presented Klamath street view, not black, not stale.
- `classic/` — the same in the game's own 2D picture:
  `SAVE_PREVIEW_FIXTURE PASS map=KLADWTWN.MAP mode=classic save_ui=1 load_ui=1 saved=1 loaded=1`
  `classic/scr00001.png` — the load dialog draws that save's picture too.
- `3d/scr00000.png` / `classic/scr00000.png` — the save screens before the
  slot was written (an empty slot's preview is black, vanilla behavior).

Every image is well under 1 MB. The fixture snapshots come from the run's
own isolated output; the owner's saves were never touched.
