# Task 157 newr1 load diagnostic

## Result

The longest standalone timed load step in the measured `NEWR1.MAP` chain was `art_preload` at 283.00 ms. Object reads were close behind at 278.67 ms for 10,472 objects. `map_objects` reached 387.12 ms cumulative, `map_elevation` 400.12 ms cumulative, and total `map_load` 749.12 ms cumulative. The sample did not report `first_frame`, so it does not explain the over-2-second load noted in the queue.

## Evidence

- Source commit: `origin/main` `b7ecc384196625233ae262be70dee4c836d15c5e`.
- Executable: previously verified Release build for `ca1a7fc10b2663c3bb874a19feeef136e3deb1e0`, an ancestor of this source commit with no source changes between them; SHA-256 `550E1FC409FD2182A666E77FAC9D0A7D109B85BF3AAF84C891E273721DBAE50D`.
- Run log: `build/test-run/runs/15ad279ba7ed/logs/devperf.log`; capture metadata identifies `NEWR1.MAP`, elevation 0. The run was isolated under the worktree's `build/test-run`.
- Invocation: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/perf-check.ps1`; this runs warmup and measured background snapshots. The measured config enables `frame_log=1`; other test defaults include 3D enabled, lights on, shadows off, clean edges on, zoom 1.0.
- Relevant second-chain figures, all in milliseconds: `ff_present_top` 46.72 cumulative; `ff_sync_done` 47.34; `ff_sharp_done` 47.78; `map_tiles` 50.37; `map_scripts` 51.14; object reads 278.67 (n=10,472); object allocation 5.08; three `objects_load` entries 242.94, 19.15, and 26.68; `map_objects` 387.12 cumulative; `map_elevation` 400.12 cumulative; `art_preload` 283.00; `map_load` 749.12 cumulative.

## Limitation

Two background preview attempts timed out. The snapshot metadata says the 3D preview was not requested and the scene log says `floor.bin` is missing or unreadable; the watchdog also reports that preview did not start and the engine capture image is missing. No `first_frame` mark was emitted. The stage numbers above are valid from the written load chain, but this run cannot identify the cause of the over-2-second first-frame load. No code or task status was changed.
