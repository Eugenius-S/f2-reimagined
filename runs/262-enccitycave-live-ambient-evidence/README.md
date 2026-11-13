# Task 262 live ambient evidence

Captured all 28 city, cave and mountain elevations with the background test game and the verified ClockLadder. Each run advances the actual game clock through Ctrl+T and emits 22:00, 02:00, 06:00 and 09:00 frames. No `--dev-hour` or `--dev-ambient` argument was used.

## Evidence

- [28 matched 3D night/day pairs](overview-28-elevations.jpg)
- [Matched original 2D and 3D views for city, mountain and cave](matched-2d-3d-city-mountain-cave.jpg)
- [Pixel diff table for every 3D frame and the 12 representative 2D frames](ambient-pixel-diffs.csv)

The JPEG sheets are each under 1 MB. Raw PNGs and JSON sidecars remain local under `build/262/fresh3d/<map-eN>/` and `build/262/classic2d/<map-e0>/`.

## Capture validation

The 28 3D runs each passed `CLOCK_LADDER PASS`. Their sidecars match map, elevation, view tile, camera, resolution (1920x1080 drawable, 960x540 logical, scale 2), scene-gl renderer, and relevant settings across all four times. The pixel diff compares the shared upper 880 pixels, excluding the HUD, and counts pixels where any RGB channel changes by more than 8.

The representative original 2D runs use city1-e0, mountn1-e0 and cave0-e0, at the same map, elevation, view tile, scale and resolution as their 3D pairs. The sidecars report `view=vanilla-framebuffer`; each 2D frame's actual game time and ambient match its 3D frame.

## Findings and limits

Visible 3D night-to-day changes occur on 21 of 28 elevations. They all change from ambient 28426 at 22:00 to 65536 by 06:00. Seven cave elevations remain visually unchanged and retain ambient 28426 through 09:00: cave1-e1, cave2-e1, cave3-e1, cave4-e1, cave5-e2, cave6-e0 and cave7-e0. A cave's unchanged image is not treated as an error by itself.

The three matched original 2D examples show the same night-to-day ambient transition. Visual review of the 28-elevation sheet and full-size city1-e0, cave0-e0 and mountn1-e0 frames found textured, covered ground and no white-floor capture in this task's target maps. No 3D-only defect was identified.

Task 270 separately confirmed a white ground in the bake-enabled KLAMALL capture even with a fresh cache; its paired `F2CE_NO_BAKE=1` diagnostic shows textured ground. That KLAMALL frame is not used as valid evidence here, and the issue was not reproduced in the representative or target-map frames for task 262. Its cause remains outside this evidence task.

Build: Release from commit `c3b98289c`, executable SHA-256 `418d642bbcf00a9bc7bb7a7e02143f85261eef90e08826984a32782b5acf86c6`.
