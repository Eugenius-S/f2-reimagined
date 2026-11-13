---
id: 270
title: A 3D capture never comes out with white ground
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Find why `tools/dev-snapshot.ps1` on KLAMALL e0 at view tile 28096 gave a 3D frame with white ground (256, PR #455), and fix the cause or make the run fail.

## Context

Both 3D attempts in 256 (`-Still -Shadows`, then `-NoPreview`) show white ground where the floor should be. Suspects: a stale or half-built scene cache for the map, a missing `tiles.png`, the capture taken before the floor texture is uploaded.

## Acceptance

- The cause is named in the Log with the evidence.
- The same command gives a frame with the floor, saved in `runs/270-white-ground-capture/`; or, if the cache is at fault, the harness rebuilds it or stops with a message.
- With that frame, say in 256's Log whether the notched shadow edge shows.

## Out of scope

Renderer and map data changes.

## Log

- 2026-11-13 claude: cut from the review of gpt's run of 2026-11-12: the blocked evidence tasks share a few harness gaps.

### 2026-11-13 gpt: white ground isolated to bake-enabled capture, exact cause unresolved

Done: rebuilt a fresh Release worktree scene cache and captured KLAMALL.MAP e0 at tile 28096, hour 8, zoom 1. The original `-Still -Shadows` command still produced a white ground. A diagnostic capture with the same arguments and process environment `F2CE_NO_BAKE=1` displayed textured ground. The evidence isolates the failure to the bake-enabled render path, but does not identify the precise renderer fault. Fresh cache and missing floor tile resources are ruled out: exporter 1.13.0 rebuilt `klamall e0` with fingerprint `28b428f002fceaf19a591888067cdb68f513747e6952a020ad32cb8e95ec5e94`; `floor.bin` is F2FL version 7, 100x100, 369 atlas cells; `tiles.png` is 2020x1919; `art/tiles/carpt02.frm` resolves from `mods/rpu.dat`; ground atlas cell 17 contains dark textured pixels. The bake-enabled frame has 1,294,230 near-white pixels of 2,073,600 (62.4%); the bake-disabled diagnostic has 0.

Next: renderer bake diagnosis/fix is required before the unmodified original command can produce a valid floor frame. Keep this task blocked; renderer changes are out of scope here. The diagnostic frame does not make the original command pass. In the 256 PR Log, record that this bake-disabled frame shows textured ground but the reported notched shadow edge is not confidently distinguishable.

Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release`; `py -3 tools/scene_export/export.py --game-root E:\Projects\fallout2-ce-3d\game --maps klamath --cache-root build/scene-cache`; `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map klamall -Elevation 0 -ViewTile 28096 -Hour 8 -Zoom 1 -Still -Shadows -OutputDirectory build/task270-capture`; repeated with `F2CE_NO_BAKE=1` and output `build/task270-no-bake`. Both snapshots report KLAMALL.MAP e0, tile 28096, 1920x1080, 3D scene available. Evidence sheet: `runs/270-white-ground-capture/comparison.md`; raw PNG and JSON sidecars remain under `build/task270-capture/` and `build/task270-no-bake/`. No renderer or map data changed.

Not checked: exact renderer defect, original-command repair, CI, `ctest`, or whether the notched edge appears in the diagnostic view. No code changed.
