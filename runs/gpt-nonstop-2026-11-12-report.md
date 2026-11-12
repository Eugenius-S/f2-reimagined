## 2026-11-12 GPT progress block (five tasks)

- Task 145, PR #421: high effort, blocked. Checked: local Release build and `ctest` (27/27), plus the settings toggle check. Not checked: cell-by-cell behavior; Artemple has no floor cache. Blocker: missing Artemple floor cache prevents the required map proof.
- Task 140, PR #424: medium, raised to high, blocked. Checked: Release build and `ctest` (27/27); three dialogue-answer probe attempts. Not checked: English and Russian answer frames. Blocker: none of the three attempts produced a valid cursor position.
- Task 156, PR #425: high effort, blocked. Checked: source audit. Not checked: live display-change behavior. Blocker: normal reinitialization resets the map, and no runtime proof is available.
- Task 207, PR #426: medium effort, merged. Checked: Release build, `ctest` (27/27), and a background `dev-snapshot` window. Not checked: live Borderless behavior and minimize/restore. Those owner-visible checks moved to owner task 176.
- Task 247, PR #416: medium search effort, then high implementation effort, blocked. Checked: Release build and `ctest` (28/28); final classic Original/RP frames were captured and visually differ. Regression (154/154) was run before the latest UI/fallback changes, not on the final binary. Not checked: UI selection and persistence, and exact 3D HUD appearance. Blocker: UI selection/persistence and the exact 3D HUD appearance remain unverified.

Additional context, excluded from the five-task count: tour tool PR #423 merged; task 229 PR #417 and task 230 PR #418 remain blocked after 10 generations each.

Verified build: `f2r-verified/version.txt` last line at report time names commit `3cde6966664b5934d644915901ac8c8f65756f00`; `perf-check` FPS 56.6/55.8/54.0/56.4 and map loads 1939/2153/1607/989 ms (kladwtwn, newr1, vctyctyd, klatoxcv e2).
## 2026-11-12 GPT progress block (five tasks)

- Task 229, PR #417: medium effort; controller ran image generation for 10 attempts total, blocked. Checked: compared generated tanker references across attempts; tanker forms changed between outputs. Not checked: an accepted view matching the in-game silhouette, proportions, and major-detail count. Blocker: no stable tanker form matched the reference within 10 attempts.
- Task 230, PR #418: high effort; image generation ran up to 10 attempts, blocked. Checked: compared generated frames for pose and foot placement; these changed across attempts. Not checked: a frame matching the source pose, feet, silhouette, and proportions. Blocker: no acceptable source-matched frame was produced within the attempt limit.
- Task 257, PR #452: medium effort (raised from low), merged. Checked: matched Release `dev-snapshot` captures for KLATRAP and KLATOXCV at `ambient=65536`; comparison reported 0 changed pixels. Not checked: other maps and ambient settings.
- Task 264, PR #453: low effort, merged. Checked: documentation is UTF-8 and the documented Python example ran successfully. Not checked: runtime behavior beyond the example.
- Task 255, PR #454: medium effort (raised from low), merged. Checked: visually compared frames at 0/45 degrees with the original reference; pixel regression not run. Not checked: other camera angles and maps.

Verified build: `f2r-verified/version.txt` last line at report time:
`b3820c487b5f3c0222032fa656e6060598faafe3 2026-11-12 Release ctest 27/27 devshot OK; perf-check red as on main: avg 59.2/54.8/60.4/59.1 fps, load 1787/2303/1597/833 ms (kladwtwn, newr1, vctyctyd, klatoxcv e2); 1% low under 60 in background runs`
