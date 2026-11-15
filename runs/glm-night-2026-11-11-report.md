# glm's night, 2026-11-11: the queue report

Per the queue in `runs/glm-night-2026-11-11.md`, top to bottom.

- **246 (the five hanging maps): PR #403, merged.** Root cause: a script on
  each map requests combat right after the load; turn-based combat sits in
  the engine's nested input loop waiting for the player forever, so a
  scripted background run has no frames and the watchdog kills it. Fix:
  background captures drop script combat requests
  (`devtools::skipScriptCombatRequest`), the combat fixture is exempt via
  its attack window. Verified: `tools/dev-snapshot.ps1` on all five maps,
  classic plus 3D at hour 12 and 18 (15 runs, every one took its snapshot);
  `ctest` 27/27. Not verified: a player's own game on those maps (no human
  played; the code path for a player is unchanged).

- **249 (the board script): PR #404, merged.** One testable decision,
  `Resolve-LockState`: a task is live on an open PR title or a fresh
  `task/NNN-*` branch; blocked tasks print as a block with reasons; foreign
  statuses warn. Verified: `-SelfTest` (4 cases), `-Next glm`/`-Next gpt`
  with PR #401 open (186 not offered), the plain board's warning and
  `Blocked tasks: 8`, the no-`gh` fallback. Not verified: nothing within
  the acceptance.

- **250 (the frame log): PR #405, merged.** The performance counter (sub-
  millisecond rows), a fresh interval on open, the `load` column (the read
  progress handler's presents), the summary as a pure function with a
  ctest case. Verified: `ctest` 27/27 with the new `fps_counter_tests`; a
  background kladwtwn run's fresh `frame_log.csv` (1800 rows, 168 load
  rows, play summary avg 103.7 fps / 1% low 28.0 / worst 336.0 ms).
  Not verified: the engine-printed summary in the same run (background
  runs end by a process kill, not a clean `gameExit`, so the numbers come
  from the same formula the test pins).

- **251 (perf-check): PR #406, merged.** `tools/perf-check.ps1`: two
  background runs per map (kladwtwn, newr1, vctyctyd, klatoxcv e2), one
  table, non-zero exit naming the failing cell; `-SelfTest` on saved
  samples. Verified: `-SelfTest`; the first run on main (the table in the
  task Log; every 1% low under 60 in a background run, newr1 load 2300.9
  ms over 2000, exit 1 naming cells); `-MinFps 1000` fails the same way.
  Not verified: nothing; the bars guard regressions, they do not prove the
  owner's window (157).

- **252 (CI runs its tests): PR #407, merged.** The exporter job also runs
  the `tools/regression` unit tests (12) and the public-content tests (6);
  AGENTS.md says the pixel regression runs by hand, not in CI. Verified:
  the deliberately broken assertion gave a red exporter job (run
  42878708747), the revert gave a green one (run 42880103335). Not
  verified: nothing.

- **146 (the combat fixture): PR #408, draft, blocked.** Verified:
  kladwtwn 3D gives START, four CAPTUREs, PASS (`COMBAT_FIXTURE PASS
  captures=4` on the clean build); the strip
  `runs/146-combat-fixture/kladwtwn-3d-four-phases.jpg` (502 KB); ctest
  27/27. Blocker, one phrase: the aim pick (`combatFixturePointAtTarget`)
  finds no critter under the crosshair in klatoxcv 3D (the static
  preview's scene has no live critter cards) and in kladwtwn classic (the
  iso pick at the tileToScreenXY spot), and klatoxcv classic gets
  `to-hit` 0 where 3D gives 14. What is not verified: the klatoxcv and
  classic legs of the acceptance.

- **254 (tools and board hygiene): PR #409, merged.** 35 one-shot tools
  deleted; the long dash banned in `check_public_content.py` with a test
  and cleaned from 13 files; task 225's `suggested` reads `any`;
  owner_shot's encode threads joined. Verified: the U+2014 grep empty,
  unittest discovery (tools 7, regression 12, scene_export 145), the board
  prints, `ctest` 27/27, a background launch check. Not verified: nothing.

- **193 part A (the message window's clipped text): PR #410, draft,
  stopped at the night boundary.** Verified: the cause documented - the fix
  lives in the sharp text renderer (`ce/src/scene_renderer/text_draw.cc`),
  which the night queue forbids (157's speed work). Not verified: nothing
  was changed. Next session with renderer access shifts the block right
  and re-levels the wrapped lines.

- **253 (verified builds only): PR #411, merged.** `publish-latest` runs
  only on `workflow_dispatch` with the commit as its input. Verified: the
  real dispatch run 42897800793 (windows-game + publish-latest green) and
  the release `latest`'s `version.txt` = `96416cf8...`, the same commit
  recorded in `f2r-verified/version.txt`; a docs-only merge publishes
  nothing (the job skips on push/PR). Not verified: the launcher's own
  install from that release.

Reserve items 225, 247, 130, 148 and 111 were not started (the night spent
its budget on the main queue; the two drafts above carry the context).

Rule 1 was run after the last code merges: the verified build in
`f2r-verified` is commit `96416cf847279fecf6fe07b509948bb1cb9d3123` (253's
merge; Release, `ctest` 27/27, one green background devshot, the perf-check
self-test green, the players' `latest` release dispatched for exactly that
commit).
