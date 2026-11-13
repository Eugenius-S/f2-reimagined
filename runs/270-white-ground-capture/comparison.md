# Task 270 capture comparison

Both images are from the background test build at KLAMALL.MAP e0, view tile 28096, hour 8, zoom 1, 1920x1080. The first is the original requested command. The second adds only the process environment override `F2CE_NO_BAKE=1` to isolate the rendering path. Raw PNG and JSON files are local under `build/task270-capture/` and `build/task270-no-bake/`.

| Capture | Bake path | Ground | Near-white pixels | Sidecar |
| --- | --- | --- | ---: | --- |
| [Original command](bake-enabled-white-ground.png) | enabled by `-Shadows` | white | 1,294,230 / 2,073,600 (62.4%) | `build/task270-capture/devshot-20261113-194248-000.json` |
| [Diagnostic](no-bake-floor-visible.jpg) | disabled by `F2CE_NO_BAKE=1` | textured | 0 / 2,073,600 | `build/task270-no-bake/devshot-20261113-194607-000.json` |

The diagnostic isolates the failure to the bake-enabled rendering path but does not prove the precise renderer fault. The original command still fails visually; task 270 remains blocked pending renderer diagnosis, which is out of scope here.
