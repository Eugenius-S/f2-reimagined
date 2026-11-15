# Task 257: Klamath point-light normal ambient evidence

The historical shader pair changes only `sceneLighting` mode 2: the fixed shader reads `gFaceCross` captured before alpha discard; the historical shader derives the normal from `dFdx/dFdy(vWorld)`. The temporary baseline edit was restored. No runtime source change is part of this task.

## Matched views

Both comparisons use Release test builds from the same worktree commit, the same map, camera, tile, resolution, scale, game settings, and enabled scene lights. The only binary difference is the shader line above. `light` is `enabled` in all sidecars. The red-topped Klamath barrel is visible near the center of the KLATRAP frame. The toxic green slime and nearby cutout rock walls are visible in the KLATOXCV frame.

| Candidate | Map / elevation / view tile | Ambient | Changed RGBA pixels | Changed RGB channel values | Visual check |
|---|---|---:|---:|---:|---|
| Barrel | KLATRAP / 0 / 19721 | 65536 (1.0) | 0 | 0 | Barrel visible; no shader difference to crop |
| Slime | KLATOXCV / 1 / 22924 | 65536 (1.0), explicit dev snapshot override | 0 | 0 | Slime and cutout walls visible; no shader difference to crop |

The unmodified KLATOXCV map ambient at this location is 28426. At that separate natural setting the same shader pair changes 200 pixels, but that is not the requested ambient 1.0 result. At ambient 1.0 neither checked Klamath candidate has measurable changes, so there is no changed-pixel crop to inspect.

## Evidence

- [KLATRAP barrel before/after](barrel-before-after.jpg)
- [KLATOXCV slime before/after](slime-before-after.jpg)
- [KLATRAP before metadata](barrel-before.json), [after metadata](barrel-after.json)
- [KLATOXCV before metadata](slime-before.json), [after metadata](slime-after.json)

The source PNGs and per-run logs remain in this worktree's ignored `build/` folders. The committed sheets show the matched full-resolution captures. Each JPEG is under 1 MB.

## Checks

- Release build: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`
- Background captures: `tools/dev-snapshot.ps1` for each paired candidate; see source metadata for commands' effective settings.
- RGBA comparison: decoded PNGs from each pair, zero changed pixels at ambient 65536.
- Visual inspection: confirmed visible barrel, slime, and neighboring cutout wall geometry; no isolated changes exist because the pixel difference is empty.
