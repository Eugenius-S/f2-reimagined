# Fly-tour evidence for the quadrant sea fill (#320, task 202) — task 225

Engine change compares two built checkouts, not scene caches: both sides
see the same scene cache (a junction to the shared 111+-map cache; the
fill is drawn by the build, not the export).

- before: E-worktree at b4b54575 (the commit just before the #320 squash
  5943d865), Release build.
- after: this worktree's build (origin/main content).
- tools/tour/tour.py --before <before> --reuse, every spot of
  tools/tour/spots.json (294 spots, 130 maps, 188 map-elevation groups),
  each spot from the fly camera at its turns, still mode, full daylight.

Expected differences on this run: water-corner maps gain sea past the
corner (the fill working), sfdock's horizon sliver. Every other
difference is pinned (spot, turn, what and why) in the find register
below or in the tour sheets' changed.jpg.

Sheets: sheets/ (one JPEG per map elevation, each row a spot and each
turn a before | after pair), changed.jpg lists only the changed spots.
