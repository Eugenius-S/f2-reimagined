# Fly-tour evidence for the quadrant sea fill (#320, task 222), task 225

An engine change compares two built checkouts, not scene caches: both sides
see the same scene cache (a junction to the shared 130-map cache; the fill
is drawn by the build, not the export).

- before: a scratch checkout at b4b54575, the commit just before the #320
  squash 5943d865, Release build.
- after: a scratch checkout at 5943d865 itself, Release build, so the run
  isolates exactly the one #320 commit (main has moved on since the merge,
  a main build would mix in five days of later changes).
- `py -3 tools/tour/tour.py --before <before> --after <after> --reuse
  --skip-failed`, every spot of tools/tour/spots.json (294 spots, 130 maps,
  188 map-elevation groups), each spot from the fly camera at its turns
  (-45, 0, +45), still mode, full daylight.

## Verdict register

Every one of the 294 spots changed, and the pixel audit shows the changes
are one thing everywhere: the past-the-edge horizon band at the top of the
fly frame. Before the fill it shows the raw map beyond the water corner
(the brown wedge); after the fill that wedge is gone. Nothing inside any
map changed: same camera, sun, ambient, wall triangles and drawn objects
in every sidecar pair; the deepest changed pixel of any pair sits at
y=74 of 1080 (klamall e0), most pairs stay above y=24.

The six largest differences, judged by eye (crops under eye/):
each pair shows the same picture with the before-side wedge removed.

- klamall e0: 27.5k px, wedge (0,0)-(680,74). Expected fill.
- kladwtwn e0: 12.2k px, wedge (0,0)-(513,45). Expected fill.
- denbus2 e1: 17.6k px, wedge (0,0)-(1148,23). Expected fill.
- desert5 e1: 19.7k px, wedge (0,0)-(1171,24). Expected fill.
- ncr1 e0: 11.6k px, wedge (0,0)-(531,43), trees keep their silhouettes.
  Expected fill.
- broken1 e1: 20.5k px, wedge (0,0)-(647,48). Expected fill.

sfdock e0, the named expectation: changed (4.9k px, the horizon sliver at
(0,0)-(407,17)). Expected fill.

Findings: none. No spot changed where it should not, and no change went
unexplained.

## Skipped map elevations (cleared on retry)

Three groups hit the known devshot stall (a load-time race, task 246;
neither build has the 2026-11-11 combat-skip fix). The tool's new
--skip-failed kept the run going; one --reuse retry shot all three:

- denbus1 e2, dnslvrun e2 (before side, "No snapshot within 144 s"),
- navarro e1 (after side, "No snapshot within 132 s").

Their shots show the same top-strip-only pattern (max diff y 19, 23, 46).

## Files

- sheets/: 227 JPEG pages, one per map elevation (rows are spots, each
  turn a before | after pair) plus changed-*.jpg with every changed spot
  framed in red.
- eye/: the six largest pairs, zoomed.
- tour-run.log, skipped.log: the run record, machine paths scrubbed.

Tool note: tour.py gained --skip-failed for this run (a group whose
devshot fails keeps no stamp, so a later --reuse run shoots it again);
without it the run would die on the first stalled map.
