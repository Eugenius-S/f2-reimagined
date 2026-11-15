# Task 263 route attempts

No further attempts were made after the two previously documented methods failed for each target. The original sidecars are preserved locally under `build/task263-sidecars/runs/263-encspecial-live-ambient-evidence/route-attempts/`.

- `encfite e1`, direct `-Map encfite -Elevation 1 -ViewTile 20100 -ClockLadder`: every accepted frame reports `ENCFITE.MAP e0`.
- `encfite e1`, map-tour `-Map encfite -Elevation 0 -Tour 'encfite:1:20100'`: both frames report `ENCFITE.MAP e0`.
- `encfite e2`, direct `-Map encfite -Elevation 2 -ViewTile 20100 -ClockLadder`: every accepted frame reports `ENCFITE.MAP e0`.
- `encfite e2`, map-tour `-Map encfite -Elevation 0 -Tour 'encfite:2:20100'`: both frames report `ENCFITE.MAP e0`.
- `cowbomb e0`, direct `-Map cowbomb -Elevation 0 -ViewTile 20100 -ClockLadder`: enter script moved to `ARTEMPLE.MAP e0`; the watchdog stopped at 30 seconds without an image.
- `cowbomb e0`, map-tour from `cardesrt`: both frames report `CARDESRT.MAP e0`.

No route enters `encfite e1/e2` or remains on cowbomb. This limitation is documented without counting a different map as proof.
